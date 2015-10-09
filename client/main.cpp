#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>

extern "C" {
  #include <sp.h>
  #include <unistd.h>
}

#include "../slave/fileop.h"

using namespace std;

const char *kGroup = "CLIENTS";

void RequestCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME]);
void RequestRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME]);
void RequestRead(mailbox &mbox, const char sender[MAX_GROUP_NAME]);
void RequestWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME]);

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

void RequestCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME]) {
  static std::string filename;
  cout << "Please provide a file name: ";
  cin >> filename;
  string create_msg("\"");
  create_msg.append(filename).append("\"");
  auto ret = SP_multicast(mbox, SAFE_MESS, sender, kFileCreate,	create_msg.size(), create_msg.c_str());
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

void RequestRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME]) {
  static std::string filename;
  cout << "Please provide a file name: ";
  cin >> filename;
  string remove_msg("\"");
  remove_msg.append(filename).append("\"");
  auto ret = SP_multicast(mbox, SAFE_MESS, sender, kFileRemove,	remove_msg.size(), remove_msg.c_str());
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

void RequestRead(mailbox &mbox, const char sender[MAX_GROUP_NAME]) {
  static std::string filename;
  cout << "Please provide a file name: ";
  cin >> filename;
  string read_msg("\"");
  read_msg.append(filename).append("\"");
  auto ret = SP_multicast(mbox, SAFE_MESS, sender, kFileRead,	read_msg.size(), read_msg.c_str());
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

void RequestWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME]) {
  static std::string filename;
  cout << "Please provide a file name: ";
  cin >> filename;
  string write_msg("\"");
  write_msg = write_msg + filename + "\"";
  cout << "Please provide the content to be written: ";
  cin >> filename;
  write_msg.append(filename);
  auto ret = SP_multicast(mbox, SAFE_MESS, sender, kFileWrite,	write_msg.size(), write_msg.c_str());
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

  char sender[MAX_GROUP_NAME] = "#r6055-10#localhost";

  bool run = true;
  while (run) {
    switch(ReadUserOperation()) {
    case kFileCreate:
      RequestCreate(mbox, sender);
      break;
    case kFileRemove:
      RequestRemove(mbox, sender);
      break;
    case kFileRead:
      RequestRead(mbox, sender);
      break;
    case kFileWrite:
      RequestWrite(mbox, sender);
      break;
    default:
      cout << "Goodbye." << endl;
      run = false;
      break;
    }
  }
  SP_disconnect(mbox);
}
