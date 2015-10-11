#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
extern "C" {
  #include <sp.h>
  #include <unistd.h>
}

#include "../slave/fileop.h"

using namespace std;
using namespace std::chrono;

const char* kGroup = "SERVER_GROUP";

// Bully
int gId;
const double kTimeout = 1000;// timeout in seconds for the bully election to take place
const double kCheckLeaderTimeout = 5000;
const short kElectionMessage = 0xDEAD;
const short kAnswerMessage = 0xCAFE;
const short kCoordinatorMessage = 0xBEEF;

const short kJoinMessage = 0xABBA;
const short kGreetMessage = 0xBAAB;

const short kPingMessage = 0x0474;
const short kPongMessage = 0x4740;

const short kNoMessage = 0xFFFF;

struct Message {
  int16_t type;
  string sender;
  vector<char> data;
};


void PrintMember(const string &member) {
  cout << "ID " << member << endl;
}

void PrintMembers(const vector<string> &members) {
  for (auto &member : members){
    PrintMember(member);
  };
}

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
  int num_groups, endian;
  int sv_type = 0, ret;
  Message response;
retry:
  ret = SP_receive(mbox, &sv_type, sender, 32, &num_groups,
                   groups, &response.type, &endian,
                   response.data.size(), response.data.data());
  response.sender = sender;
  if (BUFFER_TOO_SHORT == ret) {
    response.data.resize(-endian);
    goto retry;
  } else if (ret < 0) {
    response.type = kNoMessage;
  }
  cout << "Sender: " << response.sender << endl;
  return response;
}

void SpreadRun() {
  sp_time timeout{5, 0};
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", to_string(gId).c_str(), 0, 0, &mbox, group, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }
  string me = group; // this process private group
  cout << "Me: " << me << endl;
  if (SP_join(mbox, kGroup)) {
    cout << "Failed to join " << kGroup << endl;
    return;
  }
  SP_multicast(mbox, SAFE_MESS, kGroup, kJoinMessage, 0, "");

  string leader = me;
  vector<string> others;
  bool election = false;
  auto election_start = steady_clock::now();
  auto last_check_leader = steady_clock::now();
  bool checking_leader = false;
  auto start_election = false;
  while (true) {
    if (duration_cast<milliseconds>(steady_clock::now()-last_check_leader).count() > kCheckLeaderTimeout) {
      last_check_leader = steady_clock::now();
      if (me == leader) continue;
      if (checking_leader) {
        cout << "Leader Died" << endl;
        start_election = true; // leader died
        checking_leader = false;
      } else {
        cout << "Checking Leader: " << SP_multicast(mbox, SAFE_MESS, leader.c_str(), kPingMessage, 0, "") << endl;
        checking_leader = true;
      }
    }
    if (election && duration_cast<milliseconds>(steady_clock::now()-election_start).count() > kTimeout) {
      SP_multicast(mbox, SAFE_MESS, kGroup, kCoordinatorMessage, 0, "");
      leader = me;
      election = false;
    }
    if (!election && start_election) {
      start_election = false;
      for(auto &m: others) {
        try {
          if (stoi(me.substr(1)) < stoi(m.substr(1))) // Ignoring the # char
            SP_multicast(mbox, SAFE_MESS, m.c_str(), kElectionMessage, 0, "");
        } catch(...) {}
      }
      election_start = steady_clock::now();
      election = true;
    }
    if (SP_poll(mbox) <= 0) continue;
    auto msg = GetMessage(mbox);
    if (msg.sender == me) continue;
    switch(msg.type) {
    case kJoinMessage:
      others.push_back(msg.sender);
      SP_multicast(mbox, SAFE_MESS, msg.sender.c_str(), kGreetMessage, 0, "");
      start_election = true;
      break;
    case kGreetMessage:
      others.push_back(msg.sender);
      break;
    case kElectionMessage:
      SP_multicast(mbox, SAFE_MESS, msg.sender.c_str(), kAnswerMessage, 0, "");
      start_election = true;
      break;
    case kAnswerMessage:
      election = false;
      this_thread::sleep_for(seconds(1));
      break;
    case kCoordinatorMessage:
      leader = msg.sender;
      cout << "New Coordinator: " << leader << endl;
      break;
    case kPingMessage:
      cout << "Received Ping." << endl;
      SP_multicast(mbox, SAFE_MESS, msg.sender.c_str(), kPongMessage, 0, "");
      break;
    case kPongMessage:
      cout << "Leader Alive." << endl;
      checking_leader = false;
      break;
    default:
      cout << "Spurious message received." << endl;
      continue;
    }

    PrintMembers(others);
    cout << "LEADER: " << leader << endl;
  }
  SP_leave(mbox, kGroup);
  SP_disconnect(mbox);
}

int main(int argc, char **argv) {
  if (!ParseArgs(argc, argv)) {
    return 1;
  }
  SpreadRun();

  return 0;
}
