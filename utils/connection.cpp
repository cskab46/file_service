#include "connection.h"

#include <chrono>
using namespace std::chrono;

const int kConnectTimeout = 5;
const int kMaxGroupsPerMsg = 32;

Connection::Connection(const mailbox &mbox, const string &identifier)
  : mbox_(mbox),
    identifier_(identifier) {}

Connection Connection::Connect(bool &err, bool membership_msgs) {
  return Connect("", err, membership_msgs);
}

Connection Connection::Connect(const string &name, bool &err, bool membership_msgs) {
  return Connect("", name, err, membership_msgs);
}

Connection Connection::Connect(const string &server, const string &name, bool &err, bool membership_msgs) {
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout(server.c_str(), name.c_str(), 0, membership_msgs ? 1 : 0,
                                &mbox, group, sp_time{kConnectTimeout,0});
  err = (ACCEPT_SESSION != ret);
  return Connection(mbox, group);
}

Connection::~Connection() {
  SP_disconnect(mbox_);
}

string Connection::identifier() const {
  return identifier_;
}

bool Connection::JoinGroup(const string &group) {
  return 0 == SP_join(mbox_, group.c_str());
}

bool Connection::HasMessage() {
  return SP_poll(mbox_) > 0;
}

const Message Connection::GetMessage() {
  char sender[MAX_GROUP_NAME];
  char groups[kMaxGroupsPerMsg][MAX_GROUP_NAME];
  int num_groups, endian, service, ret;
  int16_t type;
  size_t buffer_size = 1;
  void *buffer = malloc(buffer_size);
retry:
  ret = SP_receive(mbox_, &service, sender, kMaxGroupsPerMsg, &num_groups,
                   groups, &type, &endian,
                   buffer_size, (char*)buffer);
  if (BUFFER_TOO_SHORT == ret) {
    buffer_size = -endian;
    buffer = realloc(buffer, buffer_size);
    goto retry;
  } else if (ret < 0) {
    type = kCorruptedMessage;
  }
  string data((char*)buffer, buffer_size);
  free(buffer);
  vector<string> tmp;
  for (int i = 0; i < num_groups; i++) tmp.push_back(groups[i]);
  Message msg(type, service);
  msg.set_data(data);
  msg.set_group(tmp);
  msg.set_sender(sender);
  return msg;
}

bool Connection::GetMessage(int16_t type, unsigned int timeout, Message &msg) {
  auto start = steady_clock::now();
  bool got = false;
  while (!got && duration_cast<milliseconds>(steady_clock::now() - start).count()
         < timeout) {
    if (!HasMessage()) continue;
    auto tmp = GetMessage();
    if (got = (tmp.type() == type)) {
      msg = tmp;
    }
  }
  return got;
}

bool Connection::GetMessage(unsigned int timeout, Message &msg) {
  auto start = steady_clock::now();
  while (duration_cast<milliseconds>(steady_clock::now() - start).count()
         < timeout) {
    if (!HasMessage()) continue;
    msg = GetMessage();
    return true;
  }
  return false;
}

bool Connection::SendMessage(const Message &msg, const string &to) {
  auto data = msg.data();
  return SP_multicast(mbox_, msg.service(), to.data(), msg.type(),
                      data.size(), data.data()) >= 0;
}
