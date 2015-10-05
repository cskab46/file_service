#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <thread>
#include <algorithm>
extern "C" {
  #include <sp.h>
  #include <unistd.h>
}

using namespace std;

const string kNamePrefix = "SERVER_";
const char* kGroup = "SERVER_GROUP";
const short kInfoMessageType = 0xDEAD;
const short kVoteMessageType = 0xCAFE;

struct Server {
  string name;
  unsigned int id;
  unsigned int priority;
};

unsigned int gId;
unsigned int gPriority;
bool gLeave = false;

void PrintMember(const Server &member) {
    cout << "(" << kNamePrefix << member.id <<
            "; PRIORITY: " << member.priority << ")" << endl;
}

void PrintMembers(const vector<Server> &members) {
  for (auto &member : members) {
    PrintMember(member);
  }
}

Server ChooseCandidate(const vector<Server> &members) {
  auto great = [](const Server &s1, const Server &s2) {
    return s1.priority < s2.priority;
  };

  auto leader = max_element(begin(members), end(members), great);

  return leader == end(members) ? Server {"", 0,0} : *leader;
}

string InfoMessage() {
  stringstream tmp;
  tmp << gId << "-" << gPriority;
  return tmp.str();
}

string CandidateMessage(const Server &leader) {
  stringstream tmp;
  tmp << leader.name << "-" << leader.id << "-" << leader.priority;
  return tmp.str();
}

string CandidateFromMessage(const string &msg) {
  auto sstr = msg.substr(0, msg.find("-"));
  return sstr == msg ? "": sstr;
}

bool IdFromInfoMessage(const string &msg, unsigned int &id) {
  istringstream id_parse(msg);
  id_parse >> id;
  return !id_parse.fail();
}

bool PriorityFromInfoMessage(const string &msg, unsigned int &data) {
  istringstream id_parse(msg);
  int priority_pos = msg.find("-");
  if (priority_pos == string::npos) return false;
  id_parse.ignore(priority_pos+1);
  id_parse >> data;
  return !id_parse.fail();
}

bool ParseArgs(int argc, char **argv) {
  if (argc != 3) {
    cout << "Usage: process_group <ID> <PRIORITY>" << endl;
    return false;
  }
  istringstream parse_id(argv[1]);
  parse_id >> gId;
  if (parse_id.fail() || gId > 1000) {
    cout << "<ID>: unsigned int in range[0-1000]" << endl;
    return false;
  }
  istringstream parse_priority(argv[2]);
  parse_priority >> gPriority;
  if (parse_id.fail() || gPriority > 100) {
    cout << "<PRIORITY>: unsigned int in range[0-100]" << endl;
    return false;
  }
  return true;
}

void SpreadRun() {
  sp_time timeout{5, 0};
  mailbox mbox;
  char priv_name[MAX_PRIVATE_NAME];

  stringstream name(kNamePrefix);
  name << gId;
  auto ret = SP_connect_timeout("", name.str().c_str(), 0, 1, &mbox, priv_name, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }

  if (SP_join(mbox, kGroup)) {
    cout << "Failed to join " << kGroup << endl;
    return;
  }

  char sender[MAX_GROUP_NAME], groups[32][MAX_GROUP_NAME], msg[256];
  int num_groups, endian;
  short msg_type;

  vector<Server> members; // server private name -> server info
  map<string, int> election_box; // vote message -> count

  Server leader;
  string info = InfoMessage();
  bool elected = false;
  int voters = 0;

  while (!gLeave) {
    int sv_type = 0;
    fill(msg, msg+256, 0);
    int err = SP_receive(mbox, &sv_type, sender, 32, &num_groups,
                         groups, &msg_type, &endian, 256, msg);
    if (err < 0) {
      cout << "SP_receive error: " << err << endl;
      sleep(1);
      continue;
    }
    if (Is_reg_memb_mess(sv_type)) {
      voters = num_groups;
      elected = false;
      members.clear();
      election_box.clear();
      SP_multicast(mbox, SAFE_MESS, kGroup, kInfoMessageType,
                   info.length(), info.c_str());
      continue;
    }
    string winner, winner_vote, candidate_msg;
    Server candidate;
    switch(msg_type) {
    case kInfoMessageType:
      unsigned int id, priority;
      if (!IdFromInfoMessage(msg, id) ||
          !PriorityFromInfoMessage(msg, priority)) {
        cout << "Invalid info message format." << msg << endl;
        continue;
      }
      members.push_back(Server {sender, id, priority});
      if (members.size() == voters) {
        candidate = ChooseCandidate(members);
        candidate_msg = CandidateMessage(candidate);
        SP_multicast(mbox, SAFE_MESS, kGroup, kVoteMessageType,
                     candidate_msg.length(), candidate_msg.c_str());
      }
      break;
    case kVoteMessageType:
      if (elected) continue;
      election_box[msg]++;
      winner_vote;
      for (auto &vote: election_box) {
        if (vote.second >= (voters/2 + 1)) {
          winner_vote = vote.first;
        }
      }
      if (winner_vote.empty()) continue;
      winner = CandidateFromMessage(winner_vote);
      for (auto &member : members) {
        if (member.name == winner) {
          elected = true;
          leader = member;
          PrintMembers(members);
          cout << "LEADER: "; PrintMember(leader);
          break;
        }
      }
      break;
    default:
      cout << "Spurious message received." << endl;
      break;
    }

  }
}

int main(int argc, char **argv) {
  if (!ParseArgs(argc, argv)) {
    return 1;
  }

  SpreadRun();

  return 0;
}

