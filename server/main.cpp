#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
extern "C" {
  #include <sp.h>
  #include <unistd.h>
}

#include "fileop.h"
#include "messages.h"
#include "groups.h"

using namespace std;
using namespace std::chrono;

// Bully
int gId;
const double kTimeout = 1000;// timeout in seconds for the bully election to take place

struct Message {
  int service;
  int16_t type;
  string sender;
  vector<char> data;
  vector<string> group;
};

bool ParseArgs(int argc, char **argv) {
  if (argc != 2) {
    cout << "Usage: server <ID>" << endl;
    return false;
  }
  istringstream parse_id(argv[1]);
  parse_id >> gId;
  if (parse_id.fail() || gId > 1000) {
    cout << "<ID>: unsigned int in range[0-1000]" << endl;
    return false;
  }
  return true;
}

Message GetMessage(mailbox &mbox);

Message GetMessage(mailbox &mbox) {
  char sender[MAX_GROUP_NAME];
  char groups[32][MAX_GROUP_NAME];
  int num_groups, endian, sv_type = 0, ret;
  Message response; response.data.resize(0);
retry:
  ret = SP_receive(mbox, &sv_type, sender, 32, &num_groups,
                   groups, &response.type, &endian,
                   response.data.size(), response.data.data());
  response.sender = sender;
  response.service = sv_type;
  if (BUFFER_TOO_SHORT == ret) {
    response.data.resize(-endian);
    goto retry;
  } else if (ret < 0) {
    response.type = kNoMessage;
  }
  for (int i = 0; i < num_groups; i++) response.group.push_back(groups[i]);
  return response;
}

// Returns true if m1 has high priority than m2
bool HasHighPriority(const string &m1, const string &m2) {
  return stoi(m1.substr(1)) < stoi(m2.substr(1)); // Ignoring the # char
}
void ElectLeader(vector<string> candidates, string me, atomic<bool> &finished) {
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", NULL, 0, 0, &mbox, group, sp_time{5,0});
  if (ACCEPT_SESSION != ret) return;
  for(auto &m: candidates) {
     SP_multicast(mbox, SAFE_MESS, m.c_str(), kElectionMessage, 0, "");
  }
  bool won = true;
  auto start = steady_clock::now();
  while(duration_cast<milliseconds>(steady_clock::now() - start).count() < kTimeout) {
    if (SP_poll(mbox) > 0) {
      auto msg = GetMessage(mbox);
      if (find(begin(candidates), end(candidates), msg.sender) != end(candidates) && msg.type == kAnswerMessage) {
        won = false;
        break;
      }
    }
  }
  if (won) SP_multicast(mbox, SAFE_MESS, kLeadershipGroup, kCoordinatorMessage, me.size(), me.c_str());
  SP_disconnect(mbox);
  finished = true;
}

void Leadership(atomic<bool> &lead) {
  sp_time timeout{5, 0};
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", to_string(gId).c_str(), 0, 1, &mbox, group, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }
  string me = group; // this process private group
  cout << "Me: " << me << endl;
  if (SP_join(mbox, kLeadershipGroup)) {
    cout << "Failed to join " << kLeadershipGroup << endl;
    return;
  }
  vector<string> candidates;
  auto start_election = false;
  std::atomic<bool> election_finished(true);
  while (true) {
    if (election_finished && start_election) {
      election_finished = false;
      start_election = false;
      thread t(ElectLeader, candidates, me, ref(election_finished));
      t.detach();
    }

    if (SP_poll(mbox) <= 0) continue;
    auto msg = GetMessage(mbox);
    if (Is_reg_memb_mess(msg.service)) {
      candidates.clear();
      for (auto &m : msg.group) if (HasHighPriority(me, m)) candidates.push_back(m);
      start_election = true;
      continue;
    }
    switch(msg.type) {
    // Bully Messages
    case kElectionMessage:
      SP_multicast(mbox, SAFE_MESS, msg.sender.c_str(), kAnswerMessage, 0, "");
      start_election = election_finished;
      break;
    case kAnswerMessage:
      candidates.push_back(msg.sender);
      break;
    case kCoordinatorMessage:
      lead = (string(msg.data.data(), msg.data.size())== me);
      cout << "New Coordinator: " << string(msg.data.data(), msg.data.size()) << endl;
      break;
    default:
      cout << "Spurious message received." << endl;
      continue;
    }
  }
  SP_leave(mbox, kLeadershipGroup);
  SP_disconnect(mbox);
}

mutex slaves_mutex;
vector<string> slaves;

#include "ordered_lock.h"

mutex locks_mutex;
// file -> (slave, owner)
struct file_lock {
  string slave;
  string owner;
  ordered_lock lock;
};

map<string, file_lock> locks;

void HandleOperation(Message request) {
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", NULL, 0, 0, &mbox, group, sp_time{5,0});
  if (ACCEPT_SESSION != ret) return;
  auto file = string(begin(request.data), end(request.data));
  locks_mutex.lock();
  auto has_entry = locks.count(file);
  locks_mutex.unlock();
  string slave;
  cout << "Request " << endl;
  cout << "Type: " << request.type << endl;
  cout << "Sender: " << request.sender << endl;
  cout << "Data: " << string(begin(request.data), end(request.data)) << endl;
  switch(request.type) {
  case kFileCreate:
    slaves_mutex.lock();
    if (has_entry || slaves.empty()) {
      slaves_mutex.unlock();
      SP_multicast(mbox, SAFE_MESS, request.sender.c_str(), kFileCreateFail, request.data.size(), request.data.data());
      return;
    }
    slave = slaves[rand()%slaves.size()];
    slaves_mutex.unlock();
    locks_mutex.lock();
    locks[file].slave = slave;
    locks_mutex.unlock();
    break;
  case kFileRemove:
    if (!has_entry || slaves.empty()) {
      SP_multicast(mbox, SAFE_MESS, request.sender.c_str(), kFileRemoveFail, request.data.size(), request.data.data());
      return;
    }
    break;
  case kFileRead:
    if (!has_entry || slaves.empty()) {
      SP_multicast(mbox, SAFE_MESS, request.sender.c_str(), kFileReadFail, request.data.size(), request.data.data());
      return;
    }
    break;
  case kFileWrite:
    if (!has_entry || slaves.empty()) {
      SP_multicast(mbox, SAFE_MESS, request.sender.c_str(), kFileWriteFail, request.data.size(), request.data.data());
      return;
    }
    break;
  default:
    cout << "Spurious message." << endl;
    return;
  }

  {
    locks_mutex.lock();
    auto &f_lock = locks[file];
    locks_mutex.unlock();

    slave = f_lock.slave;
    f_lock.lock.lock();
    f_lock.owner = request.sender;
  }

  auto msg = to_string(file.size()) + "#" + file +
      to_string(slave.size()) + "#" + slave +
      to_string(request.sender.size()) + "#" + request.sender;
  SP_multicast(mbox, SAFE_MESS, kProxyGroup, kServerLogMessage, msg.size(), msg.data());

  if (SP_multicast(mbox, SAFE_MESS, request.sender.c_str(), request.type, slave.size(), slave.c_str()) > 0) {
    while(true) {
      if (SP_poll(mbox) > 0) {
        auto msg = GetMessage(mbox);
        auto response_file = string(begin(msg.data), end(msg.data));
        if (msg.sender == request.sender && file == response_file) break;
      }
    }
  }
  {
    locks_mutex.lock();
    auto &f_lock = locks[file];
    locks_mutex.unlock();
    f_lock.owner.clear();
    f_lock.lock.unlock();

    locks_mutex.lock();
    if (request.type == kFileRemove) {
      locks.erase(file);
    }
    locks_mutex.unlock();
  }
  msg = to_string(file.size()) + "#" + file +
      to_string(slave.size()) + "#" + slave +
      "0#";
  SP_multicast(mbox, SAFE_MESS, kProxyGroup, kServerLogMessage, msg.size(), msg.data());

  SP_disconnect(mbox);
}

void HandleLog(const Message &log) {
  if (kServerLogMessage == log.type) {
    auto msg = string(begin(log.data), end(log.data));
    auto first_m = msg.find_first_of('#');
    auto second_m = msg.find_first_of('#', first_m + 1);
    auto third_m = msg.find_first_of('#', second_m + 1);

    auto file_size = stoi(msg);
    auto file = msg.substr(first_m + 1, file_size);
    auto slave_size = stoi(msg.substr(first_m+1+file_size));
    auto slave = msg.substr(second_m + 1, slave_size);
    auto owner_size = stoi(msg.substr(second_m+1+slave_size));
    auto owner = msg.substr(third_m + 1, owner_size);

    locks_mutex.lock();
    locks[file].slave = slave;
    locks[file].owner = owner;
    if( owner.empty() ) {
      locks[file].lock.unlock();
    } else {
      locks[file].lock.lock();
    }
    locks_mutex.unlock();
  } else if (kClientLogMessage == log.type) { // Triggered on client release of lock
    auto msg = string(begin(log.data), end(log.data));
    auto first_m = msg.find_first_of('#');
    auto file_op = stoi(msg);
    auto file = msg.substr(first_m + 1);
    locks_mutex.lock();
    if (!locks[file].owner.empty()) { // not already released?
      locks[file].owner.clear();
      locks[file].lock.unlock();
    }
    if (kFileRemove == file_op) {
      locks.erase(file);
    }
    locks_mutex.unlock();
  } else {
    cout << "Invalid log message." << endl;
  }
}

void Slaves() {
  sp_time timeout{5, 0};
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", NULL, 0, 1, &mbox, group, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }
  if (SP_join(mbox, kSlavesGroup)) {
    cout << "Failed to join " << kSlavesGroup << endl;
    return;
  }
  while (true) {
    if (SP_poll(mbox) <= 0) {
      this_thread::sleep_for(milliseconds(100));
      continue;
    }
    auto msg = GetMessage(mbox);
    if (msg.type == kSlaveMessage) {
      slaves_mutex.lock();
      slaves.push_back(string(begin(msg.data), end(msg.data)));
      slaves_mutex.unlock();
      continue;
    } else if (!Is_reg_memb_mess(msg.service)) {
      continue;
    }
    vector<string> down;
    slaves_mutex.lock();
    for (auto &m : slaves) if (find(begin(msg.group), end(msg.group), m) == end(msg.group)) down.push_back(m);
    for (auto &d: down) slaves.erase(remove(begin(slaves), end(slaves), d), end(slaves));
    slaves_mutex.unlock();

    vector<string> del;
    locks_mutex.lock();
    for (auto &d : down) for (auto &f : locks) if (f.second.slave == d) del.push_back(f.first);
    for (auto &f: del) locks.erase(f);
    locks_mutex.unlock();
  }
  SP_leave(mbox, kSlavesGroup);
  SP_disconnect(mbox);
}

void Proxy(atomic<bool> &lead) {
  sp_time timeout{5, 0};
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", NULL, 0, 0, &mbox, group, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }
  if (SP_join(mbox, kProxyGroup)) {
    cout << "Failed to join " << kProxyGroup << endl;
    return;
  }
  while (true) {
    if (SP_poll(mbox) <= 0) continue;
    auto msg = GetMessage(mbox);
    if ((msg.type == kServerLogMessage || msg.type == kClientLogMessage) && !lead) {
      HandleLog(msg);
    } else if (lead) {
      thread t(HandleOperation, msg);
      t.detach();
    } else {
      //...
    }
  }
  SP_leave(mbox, kProxyGroup);
  SP_disconnect(mbox);
}

int main(int argc, char **argv) {
  if (!ParseArgs(argc, argv)) {
    return 1;
  }
  atomic<bool> lead(false);
  thread leadership(Leadership, ref(lead));
  thread slaves(Slaves);
  thread proxy(Proxy, ref(lead));

  leadership.join();
  slaves.join();
  proxy.join();
  return 0;
}
