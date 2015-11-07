#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>

extern "C" {
  #include <sp.h>
  #include <unistd.h>
}

#include "../server/messages.h"
#include "../server/fileop.h"
#include "../server/groups.h"

using namespace std;


void RequestCreate(mailbox &mbox, string sender, string filename);
void RequestRemove(mailbox &mbox, string sender, string filename);
void RequestRead(mailbox &mbox, string sender, string filename);
void RequestWrite(mailbox &mbox, string sender, string filename);

void SpreadRun();

int main(int argc, char **argv) {
  SpreadRun();
  return 0;
}

uint16_t GetResponse(mailbox &mbox, std::string &sender, vector<char> &msg) {
  char tmp_sender[MAX_GROUP_NAME];
  char groups[32][MAX_GROUP_NAME];
  int num_groups, endian, ret, sv_type = 0;
  short msg_type;
retry:
  ret = SP_receive(mbox, &sv_type, tmp_sender, 32, &num_groups,
                       groups, &msg_type, &endian, msg.size(), msg.data());
  if (BUFFER_TOO_SHORT == ret) {
    msg.resize(-endian);
    goto retry;
  }

  if (ret < 0) {
    cout << "SP_receive error: " << ret << endl;
    return -1;
  }
  sender = tmp_sender;
  return msg_type;
}

void RequestCreate(mailbox &mbox, string sender, string filename) {
  string create_msg("\"");
  create_msg.append(filename).append("\"");
  auto ret = SP_multicast(mbox, SAFE_MESS, sender.c_str(), kFileCreate,	create_msg.size(), create_msg.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return;
  }
  std::string responder;
  vector<char> data;
  auto result = GetResponse(mbox, responder, data);

  if (result != kFileCreateSuccess || responder != sender) {
    cout << "Failed to create the file." << endl;
    return;
  }
  cout << "File created successfully." << endl;
}

void RequestRemove(mailbox &mbox, string sender, string filename) {
  string remove_msg("\"");
  remove_msg.append(filename).append("\"");
  auto ret = SP_multicast(mbox, SAFE_MESS, sender.c_str(), kFileRemove,	remove_msg.size(), remove_msg.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return;
  }
  std::string responder;
  vector<char> data;
  auto result = GetResponse(mbox, responder, data);

  if (result != kFileRemoveSuccess || responder != sender) {
    cout << "Failed to remove the file." << endl;
    return;
  }
  cout << "File removed successfully." << endl;
 }

void RequestRead(mailbox &mbox, string sender, string filename) {
  string read_msg("\"");
  read_msg.append(filename).append("\"");
  auto ret = SP_multicast(mbox, SAFE_MESS, sender.c_str(), kFileRead,	read_msg.size(), read_msg.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return;
  }
  std::string responder;
  vector<char> data;
  auto result = GetResponse(mbox, responder, data);

  if (result != kFileReadSuccess || responder != sender) {
    cout << "Failed to read the file." << endl;
    return;
  }
  cout << "File read successfully. Content: " << endl;
  cout << string(data.data(), data.size()) << endl;
 }

void RequestWrite(mailbox &mbox, string sender, string filename) {
  string write_msg("\"");
  write_msg = write_msg + filename + "\"";
  cout << "Please provide the content to be written: ";
  cin >> filename;
  write_msg.append(filename);
  auto ret = SP_multicast(mbox, SAFE_MESS, sender.c_str(), kFileWrite,	write_msg.size(), write_msg.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return;
  }
  std::string responder;
  vector<char> data;
  auto result = GetResponse(mbox, responder, data);

  if (result != kFileWriteSuccess || responder != sender) {
    cout << "Failed to write the file." << endl;
    return;
  }
  cout << "File written successfully." << endl;
 }
string AskPermission(mailbox &mbox, int op, string file , string &responder) {
  auto ret = SP_multicast(mbox, SAFE_MESS, kProxyGroup,
                          op,	file.size(), file.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return "";
  }
  vector<char> data;
  auto result = GetResponse(mbox, responder, data);

  if (result != op) {
    cout << "Failed to get permission." << endl;
    return "";
  }
  cout << "Permission granted." << endl;
  return string(begin(data),end(data));
}

void ReleasePermission(mailbox &mbox, int op, string file, string responder) {
  auto ret = SP_multicast(mbox, SAFE_MESS, responder.c_str(),
                          op,	 file.size(), file.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return;
  }
  auto msg = to_string(op) + "#" + file;
  ret = SP_multicast(mbox, SAFE_MESS, kProxyGroup,
                          kClientLogMessage,	 msg.size(), msg.c_str());
  if (ret < 0) {
    cout << "Failed sending the request." << endl;
    return;
  }
  cout << "Permission released." << endl;

}

uint16_t ReadUserOperation() {
  cout << kFileCreate << " - Create File" << endl;
  cout << kFileRemove << " - Remove File" << endl;
  cout << kFileRead << " - Read File" << endl;
  cout << kFileWrite << " - Write File" << endl;
  cout << "Anything Else - Exit Client" << endl;
  cout << "Please, choose one of the operations: ";
  string read;
  cin >> read;
  stringstream conv(read);
  uint16_t op;
  return conv >> op ? op : -1;

}

void SpreadRun() {
  sp_time timeout{5, 0};
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", NULL, 0, 0, &mbox, group, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }

  string filename, responder, slave;

  bool run = true;
  while (run) {
    switch(ReadUserOperation()) {
      case kFileCreate:
      cout << "Please provide a file name: ";
      cin >> filename;
      slave = AskPermission(mbox, kFileCreate, filename, responder);
      if (slave.empty()) {
        cout << "Failed getting permission." << endl;
        continue;
      }
      cout << "Slave: " << slave << endl;
      RequestCreate(mbox, slave, filename);
      ReleasePermission(mbox, kFileCreate, filename, responder);
      break;
    case kFileRemove:
      cout << "Please provide a file name: ";
      cin >> filename;
      slave = AskPermission(mbox, kFileRemove, filename, responder);
      if (slave.empty()) {
        cout << "Failed getting permission." << endl;
        continue;
      }
      cout << "Slave: " << slave << endl;
      RequestRemove(mbox, slave, filename);
      ReleasePermission(mbox, kFileRemove, filename, responder);
      break;
    case kFileRead:
      cout << "Please provide a file name: ";
      cin >> filename;
      slave = AskPermission(mbox, kFileRead, filename, responder);
      if (slave.empty()) {
        cout << "Failed getting permission." << endl;
        continue;
      }
      cout << "Slave: " << slave << endl;
      RequestRead(mbox, slave, filename);
      ReleasePermission(mbox, kFileRead, filename, responder);
      break;
    case kFileWrite:
      cout << "Please provide a file name: ";
      cin >> filename;
      slave = AskPermission(mbox, kFileWrite, filename, responder);
      if (slave.empty()) {
        cout << "Failed getting permission." << endl;
        continue;
      }
      cout << "Slave: " << slave << endl;
      RequestWrite(mbox, slave, filename);
      ReleasePermission(mbox, kFileWrite, filename, responder);
      break;
    default:
      cout << "Goodbye." << endl;
      run = false;
      break;
    }
  }
  SP_disconnect(mbox);
}
