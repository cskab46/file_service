#include "server_proxy.h"

#include <iostream>
#include <thread>

using namespace std;


#include "utils/connection.h"
#include "utils/message.h"
#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/file_lock_map.h"

static FileLockMap gFileMap;

ordered_lock  gSlavesLock;
static vector<pair<string, vector<string>> gSlaves; // slaves -> files mapping


void HandleCreate(const Message &msg, Connection &con);
void HandleRemove(const Message &msg, Connection &con);
void HandleRead(const Message &msg, Connection &con);
void HandleWrite(const Message &msg, Connection &con);

void ManageSlaves(const bool &quit);


void HandleOperations(const Message &msg) {
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  cout << "Message type: " << msg.type() << endl;
  switch (msg.type()) {
  case kFileCreate:
    HandleCreate(msg, con);
    break;
  case kFileRemove:
    HandleRemove(msg, con);
    break;
  case kFileRead:
    HandleRead(msg, con);
    break;
  case kFileWrite:
    HandleWrite(msg, con);
    break;
  default:
    cout << "Spurious file operation requested." << endl;
  }
}

/**
 * @brief Proxies operations requested by clients, if lead is true. Exits when quit is true.
 * @param lead
 */
void Proxy(const bool &lead, const bool &quit) {
  bool err;
  auto con = Connection::Connect(err, true);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  if (!con.JoinGroup(kProxyGroup)) {
    cout << "Failed to join group: " << kProxyGroup << endl;
    return;
  }

  thread slaves_thread(ManageSlaves, ref(quit));
  slaves_thread.detach();

  while (!quit) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();

    if (msg.IsMembership()) {
      if (lead) {
        //todo: send state to newcomer
      }
      continue;
    }

    if (lead) {
      cout << "Request: " << msg.type() << endl;
      thread t(HandleOperations, msg);
      t.detach();
    } else {
      // Just update the state
    }
  }
}

void HandleCreate(const Message &msg, Connection &con) {
  try {
    CreateFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name <<
            " and " << op.redundancy << endl;
    gSlavesLock.lock();
    auto tmp_slv = gSlaves;
    gSlavesLock.unlock();
    if (op.redundancy > tmp_slv.size()) {
      cout << "Requested redundancy cannot be met. Available: " << tmp_slv.size() << endl;
      return;
    }

    auto slaves = vector(begin(tmp_slv), begin(tmp_slv) + op.redundancy);
    auto client = msg.sender();

    // TODO: Send request to slaves and once the request is confirmed persist information
    if (!gFileMap.CreateFile(op.file_name, slaves)) {
      cout << "Create Failed: File entry already exists." << endl;
      return;
    }


    cout << "Create Succeeded. Redundancy: " << slaves.size() << endl;
  } catch (...) {
    cout << "Invalid create request from " << msg.sender() << endl;
  }
}

void HandleRemove(const Message &msg, Connection &con) {
  try {
    RemoveFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested remove with args: " << op.file_name <<
            endl;
    if (!gFileMap.RemoveFile(op.file_name)) {
      cout << "Remove Failed: File entry does not exists." << endl;
      return;
    }
    cout << "Remove Succeeded." << endl;
  } catch (...) {
    cout << "Invalid remove request from " << msg.sender() << endl;
  }
}

void HandleRead(const Message &msg, Connection &con) {
  try {
    ReadFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested read with args: " << op.file_name <<
            endl;

    vector<string> slaves;
    if (!gFileMap.LockFile(op.file_name, slaves)) {
      cout << "Read Failed: File entry doe not exists." << endl;
      return;
    }
    cout << "Do the read." << endl;
    gFileMap.UnlockFile(op.file_name);
    cout << "Read Succeeded." << endl;
  } catch (...) {
    cout << "Invalid read request from " << msg.sender() << endl;
  }
}

void HandleWrite(const Message &msg, Connection &con) {
  try {
    WriteFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested write with args: " << op.file_name <<
            " and " << op.data << endl;

    vector<string> slaves;
    if (!gFileMap.LockFile(op.file_name, slaves)) {
      cout << "Write Failed: File entry doe not exists." << endl;
      return;
    }
    cout << "Writing the data: " << op.data << endl;
    gFileMap.UnlockFile(op.file_name);
    cout << "Write Succeeded." << endl;
   } catch (...) {
    cout << "Invalid write request from " << msg.sender() << endl;
  }
}

void ManageSlaves(const bool &quit) {
  bool err;
  auto con = Connection::Connect(err, true);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  if (!con.JoinGroup(kSlavesGroup)) {
    cout << "Failed to join group: " << kSlavesGroup << endl;
    return;
  }

  while (!quit) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();

    if (!msg.IsMembership()) {
      cout << "Spurious message in slaves group." << endl;
      continue;
    }
    auto slaves = msg.group();
    slaves.erase(remove(begin(slaves), end(slaves), con.identifier()));

    gSlavesLock.lock();
    // remove dead ones
    gSlaves.erase(std::remove_if(begin(gSlaves), end(gSlaves), [] (auto entry) {
      return find(begin(slaves), end(slaves), entry.first) == end(slaves);
    }), end(gSlaves));
    // insert new ones
    copy_if(begin(slaves), end(slaves), back_inserter(gSlaves), [] (auto slave) {
      return find_if(begin(gSlaves), end(gSlaves), [] (auto entry) {
        return entry.first == slave; }) == end(gSlaves);
    });
    sort(begin(gSlaves), end(gSlaves),
         [] (auto a, auto b) {return a.second.size() < b.second.size();});
    gSlavesLock.unlock();
  }
}
