#include "server_proxy.h"

#include <iostream>
#include <thread>

using namespace std;
using namespace chrono;


#include "utils/connection.h"
#include "utils/message.h"
#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/slave_ops.h"
#include "utils/client_ops.h"
#include "utils/file_lock_map.h"

const string kServerPrefix = "SERVER";

static FileLockMap gFileMap;

static ordered_lock  gSlavesLock;
static vector<pair<string, vector<string>>> gSlaves; // slaves -> files mapping


void HandleCreate(const Message &msg, Connection &con);
void HandleRemove(const Message &msg, Connection &con);
void HandleRead(const Message &msg, Connection &con);
void HandleWrite(const Message &msg, Connection &con);

void ManageSlaves(const bool &quit, const string &id);


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

  thread slaves_thread(ManageSlaves, ref(quit), con.identifier());
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
  FileEntry *entry = NULL;
  bool succeeded = false;
  CreateFileOp op;
  try {
    bool err;
    auto con = Connection::Connect(err);
    if (err) {
      cout << "Failed to connect with spread daemon." << endl;
      return;
    }

    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name <<
            " and " << op.redundancy << endl;
    if (gFileMap.HasFile(op.file_name)) {
      cout << "Create request failed. File already exists." << endl;
      return;
    }
    gSlavesLock.lock();
    vector<string> tmp_slv;
    for (auto &entry : gSlaves) {
      tmp_slv.push_back(entry.first);
      cout << "slave: " << entry.first << endl;
    }
//    transform(begin(gSlaves), end(gSlaves), back_inserter(tmp_slv), [](const pair<string,vector<string>> &a) {return a.first;});
    gSlavesLock.unlock();
    if (op.redundancy > tmp_slv.size()) {
      cout << "Requested redundancy cannot be met. Available: " << tmp_slv.size() << endl;
      return;
    }

    if (!gFileMap.CreateAndLockFile(op.file_name, &entry)) {
      cout << "Failed to create entry. File was created in the meantime." << endl;
      return;
    }

    vector<string> slaves(begin(tmp_slv), begin(tmp_slv) + op.redundancy);
    auto client = msg.sender();
    
    // Send message to master slave(first) to prepare for client operation
    Message slave_prep(kSlavePrepareOp, SAFE_MESS);
    slave_prep.SetContent(SlaveOp{op.file_name, client, "", slaves});
    if (!con.SendMessage(slave_prep, slaves.front())) {
      cout << "Failed to prepare slaves for operation." << endl;
      goto release_file;
    }
    // Wait slave to confirm operation and to inform the handler
    auto start = steady_clock::now();
    bool confirmed = false;
    string op_handler;
    while (!confirmed &&
           duration_cast<milliseconds>(steady_clock::now() - start).count() < kCreateTimeout) {
      if (!con.HasMessage()) continue;
      auto msg = con.GetMessage();
      if (msg.type() == kSlaveConfirmOp) {
        SlaveOp cop;
        msg.GetContent(cop);
        confirmed = (cop.client == client && cop.file_name == op.file_name);
        cout << "confirmed: " << confirmed << endl;
        op_handler = msg.sender();
        break;
      } else {
        cout << "Message type: " << msg.type() << endl;
      }
    }
    // Slaves are not responding to prepare op
    if (!confirmed) {
      cout << "Storage slaves are not responding." << endl;
      gFileMap.DestroyFile(op.file_name);
      goto release_file;
    }

    Message client_prep(kClientPrepareOp, SAFE_MESS);
    client_prep.SetContent(ClientOp{op.file_name, op_handler});
    if (!con.SendMessage(client_prep, client)) {
      cout << "Failed to prepare client for operation." << endl;
      goto release_file;
    }

    // Wait slave to confirm operation and to inform the handler
    start = steady_clock::now();
    confirmed = false;
    while (!confirmed &&
           duration_cast<milliseconds>(steady_clock::now() - start).count() < kCreateTimeout) {
      if (!con.HasMessage()) continue;
      auto msg = con.GetMessage();
      if (msg.type() == kSlaveConfirmOp) {
        SlaveOp cop;
        msg.GetContent(cop);
        confirmed = cop.client == client && cop.file_name == op.file_name;
        op_handler = msg.sender();
      }
    }

    // Slave did not confirm
    if (!confirmed) {
      cout << "Storage slaves did not confirm operation." << endl;
      Message client_fail(kClientFailOp, SAFE_MESS);
//      client_fail.SetContent(ClientOp{op.file_name, slaves.front()});
      if (!con.SendMessage(client_fail, client)) {
        cout << "Failed to inform client of failure." << endl;
      }
      goto release_file;
    }

    entry->slaves = slaves;

    Message client_confirm(kClientConfirmOp, SAFE_MESS);
    client_confirm.SetContent(ClientOp{op.file_name, slaves.front()});
    if (!con.SendMessage(client_confirm, client)) {
      cout << "Failed to inform client of file creation." << endl;
    }

    cout << "File '" << op.file_name << "' created. Main slave '"
         << slaves.front() << "'. Redundancy " << slaves.size() << endl;
    succeeded = true;
  } catch (...) {
    cout << "Invalid create request from " << msg.sender() << endl;
  }
release_file:
  if (NULL != entry) {
    entry->lock.unlock();
    if (!succeeded) gFileMap.DestroyFile(op.file_name);
  }
}

void HandleRemove(const Message &msg, Connection &con) {
  try {
    RemoveFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested remove with args: " << op.file_name <<
            endl;
    if (!gFileMap.DestroyFile(op.file_name)) {
      cout << "Remove Failed: File entry does not exists." << endl;
      return;
    }
    cout << "Remove Succeeded." << endl;
  } catch (...) {
    cout << "Invalid remove request from " << msg.sender() << endl;
  }
}

void HandleRead(const Message &msg, Connection &con) {
//  try {
//    ReadFileOp op;
//    msg.GetContent(op);
//    cout << msg.sender() << " requested read with args: " << op.file_name <<
//            endl;

//    vector<string> slaves;
//    if (!gFileMap.LockFile(op.file_name, slaves)) {
//      cout << "Read Failed: File entry doe not exists." << endl;
//      return;
//    }
//    cout << "Do the read." << endl;
//    gFileMap.UnlockFile(op.file_name);
//    cout << "Read Succeeded." << endl;
//  } catch (...) {
//    cout << "Invalid read request from " << msg.sender() << endl;
//  }
}

void HandleWrite(const Message &msg, Connection &con) {
//  try {
//    WriteFileOp op;
//    msg.GetContent(op);
//    cout << msg.sender() << " requested write with args: " << op.file_name <<
//            " and " << op.data << endl;

//    vector<string> slaves;
//    if (!gFileMap.LockFile(op.file_name, slaves)) {
//      cout << "Write Failed: File entry doe not exists." << endl;
//      return;
//    }
//    cout << "Writing the data: " << op.data << endl;
//    gFileMap.UnlockFile(op.file_name);
//    cout << "Write Succeeded." << endl;
//   } catch (...) {
//    cout << "Invalid write request from " << msg.sender() << endl;
//  }
}

void ManageSlaves(const bool &quit, const string &id) {
  bool err;
  auto name = kServerPrefix + to_string(stoi(id.substr(2)));
  auto con = Connection::Connect(name, err, true);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  if (!con.JoinGroup(kSlavesGroup)) {
    cout << "Failed to join group: " << kSlavesGroup << endl;
    return;
  } else {
    cout << "Id: " << con.identifier() << endl;
  }

  while (!quit) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();

    if (!msg.IsMembership()) {
      cout << "Spurious message in slaves group." << endl;
      continue;
    }
    auto slaves = msg.group();
    slaves.erase(remove_if(begin(slaves), end(slaves), [] (const string &slave){
      return slave.find(kServerPrefix) != string::npos;
    }), end(slaves));

    gSlavesLock.lock();
    // remove dead ones
    gSlaves.erase(std::remove_if(begin(gSlaves), end(gSlaves),
                                 [slaves] (const pair<string, vector<string>> &entry) {
      return find(begin(slaves), end(slaves), entry.first) == end(slaves);
    }), end(gSlaves));
    // insert new ones
    decltype(slaves) new_ones;
    copy_if(begin(slaves), end(slaves), back_inserter(new_ones),
            [] (const string &slave) {
      return find_if(begin(gSlaves), end(gSlaves),
                     [slave] (const pair<string, vector<string>> &entry) {
        return entry.first == slave; }) == end(gSlaves);
    });
    for (auto &new_slave : new_ones)
      gSlaves.push_back(make_pair(new_slave, vector<string>()));
//    transform(begin(new_ones), end(new_ones), back_inserter(gSlaves), [](const string &a) {return make_pair(a, vector<string>());});
    sort(begin(gSlaves), end(gSlaves),
         [] (const pair<string, vector<string>> &a,
         const pair<string, vector<string>> &b) {
      return a.second.size() < b.second.size();});
    gSlavesLock.unlock();
  }
}
