#include "server_proxy.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "utils/connection.h"
#include "utils/message.h"
#include "utils/groups.h"
#include "utils/file_ops.h"

using namespace std;

void HandleCreate(const Message &msg, Connection &con);
void HandleRemove(const Message &msg, Connection &con);
void HandleRead(const Message &msg, Connection &con);
void HandleWrite(const Message &msg, Connection &con);

struct FileLock {
  vector<string> slaves;
  vector<string> wait;
  string owner;
  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, FileLock &op, const unsigned int version) {
    ar & op.slaves;
    ar & op.wait;
    ar & op.owner;
  }
};

mutex locks_mutex;
map<string, FileLock> locks;

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
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  if (!con.JoinGroup(kProxyGroup)) {
    cout << "Failed to join group: " << kProxyGroup << endl;
    return;
  }

  while (!quit) {
    if (!con.HasMessage()) continue;

    auto msg = con.GetMessage();
    if (!lead) continue;
    cout << "Request: " << msg.type() << endl;
    thread t(HandleOperations, msg);
    t.detach();
  }
}

void HandleCreate(const Message &msg, Connection &con) {
  try {
    CreateFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name <<
            " and " << op.redundancy << endl;
  } catch (...) {
    cout << "Invalid create request from " << msg.sender() << endl;
  }
}

void HandleRemove(const Message &msg, Connection &con) {
  try {
    RemoveFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name <<
            endl;
  } catch (...) {
    cout << "Invalid remove request from " << msg.sender() << endl;
  }
}

void HandleRead(const Message &msg, Connection &con) {
  try {
    ReadFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name <<
            endl;
  } catch (...) {
    cout << "Invalid read request from " << msg.sender() << endl;
  }
}

void HandleWrite(const Message &msg, Connection &con) {
  try {
    WriteFileOp op;
    msg.GetContent(op);
    cout << msg.sender() << " requested createop with args: " << op.file_name <<
            " and " << op.data << endl;
   } catch (...) {
    cout << "Invalid write request from " << msg.sender() << endl;
  }
}
