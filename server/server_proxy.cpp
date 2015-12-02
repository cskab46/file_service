#include "server_proxy.h"

#include <iostream>
#include <thread>
#include <set>

using namespace std;
using namespace chrono;


#include "utils/connection.h"
#include "utils/message.h"
#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/slave_ops.h"
#include "utils/client_ops.h"
#include "utils/server_ops.h"
#include "utils/file_lock_map.h"

const string kServerPrefix = "SERVER";

static FileLockMap gFileMap;

static ordered_lock  gSlavesLock;
static vector<pair<string, vector<string>>> gSlaves; // slaves -> files mapping


bool HandleCreate(const Message &msg, Connection &con);
bool HandleRemove(const Message &msg, Connection &con);
bool HandleRead(const Message &msg, Connection &con);
bool HandleWrite(const Message &msg, Connection &con);

void ManageSlaves(const bool &quit, const string &id);


void HandleOperations(const Message &msg, const vector<string> &servers) {
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  bool succeeded = false;
  switch (msg.type()) {
  case kFileCreate:
    succeeded = HandleCreate(msg, con);
    break;
  case kFileRemove:
    succeeded = HandleRemove(msg, con);
    break;
  case kFileRead:
    succeeded = HandleRead(msg, con);
    break;
  case kFileWrite:
    succeeded = HandleWrite(msg, con);
    break;
  default:
    cout << "Spurious file operation requested." << endl;
  }
  if (succeeded) {
    Message state(kServerStateOp, SAFE_MESS);
    gSlavesLock.lock();
    auto slaves = gSlaves;
    gSlavesLock.unlock();
    state.SetContent(ServerOp{gFileMap, slaves});
    for (auto &server : msg.group()) {
        con.SendMessage(state, server);
    }
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
  vector<string> servers;
  while (!quit) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();

    if (msg.IsMembership()) {
      servers = msg.group();
      servers.erase(find(begin(servers), end(servers), con.identifier()));
      if (lead) {
        //todo: send state to newcomer
        Message state(kServerStateOp, SAFE_MESS);
        state.SetContent(ServerOp{gFileMap, gSlaves});
        for (auto &server : msg.group()) {
          if (server != con.identifier()) {
            con.SendMessage(state, server);
          }
        }
      }
      continue;
    }
    if (lead) {
      cout << "Request: " << msg.type() << endl;
      thread t(HandleOperations, msg, servers);
      t.detach();
    } else if (msg.type() == kServerStateOp){
      ServerOp op;
      msg.GetContent(op);
      gFileMap = op.file_map;
      gSlavesLock.lock();
      gSlaves = op.slaves;
      gSlavesLock.unlock();
    } else {
      cout << "Spurious request sent to server." << endl;
    }
  }
}

vector<string> GetFileSlaves(const string &file) {
  gSlavesLock.lock();
  vector<string> slaves;
  for (auto & slave : gSlaves) {
    auto &files = slave.second;
    if (find(begin(files), end(files), file) != end(files)) {
      slaves.push_back(slave.first);
    }
  }
  gSlavesLock.unlock();
  return slaves;
}

bool PerformOp(Connection &con, string client, string file, vector<string> slaves) {
  if (slaves.empty()) {
    return false;
  }
  // Send message to master slave(first) to prepare for client operation
  Message slave_prep(kSlavePrepareOp, SAFE_MESS);
  slave_prep.SetContent(SlaveOp{file, client, "", slaves});
  if (!con.SendMessage(slave_prep, slaves.front())) {
    cout << "Failed to prepare slaves for operation." << endl;
    return false;
  }

  // Wait slave to confirm operation and to inform the handler
  bool confirmed = false;
  string op_handler;
  Message tmp(0,0);
  if (con.GetMessage(kSlaveConfirmOp, kCreateTimeout, tmp)) {
    SlaveOp cop;
    tmp.GetContent(cop);
    confirmed = cop.client == client && cop.file_name == file;
    op_handler = tmp.sender();
  }

  // Slaves are not responding to prepare op
  if (!confirmed) {
    cout << "Storage slaves did not respond." << endl;
    return false;
  }

  Message client_prep(kClientPrepareOp, SAFE_MESS);
  client_prep.SetContent(ClientOp{file, op_handler});
  if (!con.SendMessage(client_prep, client)) {
    cout << "Failed to prepare client for operation." << endl;
    return false;
  }

  // Wait slave to confirm operation and to inform the handler
  confirmed = false;
  if (con.GetMessage(kFileOpResult, kCreateTimeout, tmp)) {
    ResultFileOp result;
    tmp.GetContent(result);
    confirmed = result.ok;
  }
  // Slave did not confirm
  if (!confirmed) {
    cout << "Storage slaves did not confirm operation." << endl;
  }
  return confirmed;
}

bool HandleCreate(const Message &msg, Connection &con) {
  bool succeeded = false;
  CreateFileOp op;
  auto client = msg.sender();
  try {
    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name
         << " and " << op.redundancy << endl;
    if (!gFileMap.LockFile(op.file_name)) {
      cout << "Failed to lock entry. File was locked in the meantime." << endl;
      goto finish;
    }
    // get slaves associated with the file
    vector<string> slaves = GetFileSlaves(op.file_name);
    if (!slaves.empty()) {
      cout << "Create request failed. File does exist." << endl;
    } else {
      gSlavesLock.lock();
      for (auto &entry : gSlaves) {
        slaves.push_back(entry.first);
        if (slaves.size() == op.redundancy) break;
      }
      gSlavesLock.unlock();
      if (slaves.size() == op.redundancy) {
        succeeded = PerformOp(con, client, op.file_name, slaves);
      }
    }
    if (succeeded) {
      // insert the file on slaves
      gSlavesLock.lock();
      for (auto & slave : gSlaves) {
        if (find(begin(slaves), end(slaves), slave.first) != end(slaves)) {
          slave.second.push_back(op.file_name);
        }
      }
      gSlavesLock.unlock();
    }

  } catch (...) {
    cout << "Invalid create request from " << msg.sender() << endl;
  }
  gFileMap.UnlockFile(op.file_name);

finish:
  Message res_msg(kFileOpResult, SAFE_MESS);
  ResultFileOp res{op.file_name, succeeded};
  res_msg.SetContent(res);
  if (!con.SendMessage(res_msg, client)) {
    cout << "Failed to inform client of op result." << endl;
  }
  return succeeded;
}

bool HandleRemove(const Message &msg, Connection &con) {
  bool succeeded = false;
  bool locked = false;
  RemoveFileOp op;
  auto client = msg.sender();
  vector<string> slaves;
  try {
    msg.GetContent(op);
    cout << msg.sender() << " requested removeop with arg: " << op.file_name
         << endl;
    if (!(locked = gFileMap.LockFile(op.file_name))) {
      cout << "Failed to lock entry. File was locked in the meantime." << endl;
      goto finish;
    }
    // get slaves associated with the file
    slaves = GetFileSlaves(op.file_name);
    if (slaves.empty()) {
      cout << "Remove request failed. File does not exist." << endl;
    } else {
      succeeded = PerformOp(con, client, op.file_name, slaves);
    }

    if (succeeded) {
      // erases the files from slaves
      gSlavesLock.lock();
      for (auto & slave : gSlaves) {
        if (find(begin(slaves), end(slaves), slave.first) != end(slaves)) {
          auto &files = slave.second;
          files.erase(remove(begin(files), end(files), op.file_name), end(files));
        }
      }
      gSlavesLock.unlock();
    }
  } catch (...) {
    cout << "Invalid create request from " << msg.sender() << endl;
  }
  if (locked) {
    gFileMap.UnlockFile(op.file_name);
  }
finish:
  Message res_msg(kFileOpResult, SAFE_MESS);
  ResultFileOp res{op.file_name, succeeded};
  res_msg.SetContent(res);
  if (!con.SendMessage(res_msg, client)) {
    cout << "Failed to inform client of op result." << endl;
  }
  return succeeded;
}

bool HandleRead(const Message &msg, Connection &con) {
  bool succeeded = false;
  bool locked = false;
  ReadFileOp op;
  auto client = msg.sender();
  vector<string> slaves;
  try {
    msg.GetContent(op);
    cout << msg.sender() << " requested readop with arg: " << op.file_name
         << endl;
    if (!(locked = gFileMap.LockFile(op.file_name))) {
      cout << "Failed to lock entry. File was locked in the meantime." << endl;
      goto finish;
    }
    // get slaves associated with the file
    slaves = GetFileSlaves(op.file_name);
    if (slaves.empty()) {
      cout << "Read request failed. File does not exist." << endl;
    } else {
      slaves.resize(1);
      succeeded = PerformOp(con, client, op.file_name, slaves);
    }
  } catch (...) {
    cout << "Invalid read request from " << msg.sender() << endl;
  }
  if (locked) {
    gFileMap.UnlockFile(op.file_name);
  }
finish:
  Message res_msg(kFileOpResult, SAFE_MESS);
  ResultFileOp res{op.file_name, succeeded};
  res_msg.SetContent(res);
  if (!con.SendMessage(res_msg, client)) {
    cout << "Failed to inform client of op result." << endl;
  }
  return succeeded;
}

bool HandleWrite(const Message &msg, Connection &con) {
  bool succeeded = false;
  bool locked = false;
  WriteFileOp op;
  auto client = msg.sender();
  vector<string> slaves;
  try {
    msg.GetContent(op);
    cout << msg.sender() << " requested write with arg: " << op.file_name
         << endl;
    if (!(locked = gFileMap.LockFile(op.file_name))) {
      cout << "Failed to lock entry. File was locked in the meantime." << endl;
      goto finish;
    }
    // get slaves associated with the file
    slaves = GetFileSlaves(op.file_name);
    if (slaves.empty()) {
      cout << "Write request failed. File does not exist." << endl;
    } else {
      succeeded = PerformOp(con, client, op.file_name, slaves);
    }
  } catch (...) {
    cout << "Invalid write request from " << msg.sender() << endl;
  }
  if (locked) {
    gFileMap.UnlockFile(op.file_name);
  }
finish:
  Message res_msg(kFileOpResult, SAFE_MESS);
  ResultFileOp res{op.file_name, succeeded};
  res_msg.SetContent(res);
  if (!con.SendMessage(res_msg, client)) {
    cout << "Failed to inform client of op result." << endl;
  }
  return succeeded;
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
    auto slaves = msg.group(); // all members of the slaves groups (includes servers)
    // removing non slaves
    slaves.erase(remove_if(begin(slaves), end(slaves), [] (const string &slave){
      return slave.find(kServerPrefix) != string::npos;
    }), end(slaves));


    gSlavesLock.lock();
    // remove dead ones
    auto first_dead = remove_if(begin(gSlaves), end(gSlaves), [slaves] (const pair<string, vector<string>> &entry) {
      return find(begin(slaves), end(slaves), entry.first) == end(slaves);
    });
    decltype(gSlaves) dead_slaves(first_dead, end(gSlaves));
    gSlaves.erase(first_dead, end(gSlaves));

    set<string> lost_replicas, lost_files;
    for (auto & slave : dead_slaves) {
         for (auto & replica : slave.second) {
           lost_replicas.insert(replica);
         }
    }

    for (auto & replica : lost_replicas) {
      bool found = false;
      for (auto & slave : gSlaves) {
        if (find(begin(slave.second), end(slave.second), replica) != end(slave.second)) {
          found = true;
          break;
        }
      }
      if (!found) {
        lost_files.insert(replica);
      }
    }

    // insert new ones
    decltype(slaves) new_ones;
    copy_if(begin(slaves), end(slaves), back_inserter(new_ones),
            [] (const string &slave) {
      return find_if(begin(gSlaves), end(gSlaves),
                     [slave] (const pair<string, vector<string>> &entry) {
        return entry.first == slave; }) == end(gSlaves);
    });
    for (auto &new_slave : new_ones)
    gSlaves.emplace_back(new_slave, vector<string>());
//    transform(begin(new_ones), end(new_ones), back_inserter(gSlaves), [](const string &a) {return make_pair(a, vector<string>());});
    sort(begin(gSlaves), end(gSlaves),
         [] (const pair<string, vector<string>> &a,
         const pair<string, vector<string>> &b) {
      return a.second.size() < b.second.size();});
    gSlavesLock.unlock();

    for (auto &file : lost_files) {
      gFileMap.DestroyFile(file);
    }
  }
}
