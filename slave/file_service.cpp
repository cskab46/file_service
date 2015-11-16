//#include <iostream>
//#include <vector>
//#include <cstring>

//extern "C" {
//  #include <sp.h>
//  #include <unistd.h>
//}

//#include "fs.h"
//#include "utils/file_ops.h"
//#include "utils/groups.h"

//using namespace std;

//void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size);
//void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
//void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
//void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);

//void SpreadRun();

//int main(int argc, char **argv) {
//  SpreadRun();
//  return 0;
//}

//void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size) {
//  if (size<3) goto fail;
//  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
//  if (!CreateFile(string(msg+1, size-2))) goto fail;

//  SP_multicast(mbox, SAFE_MESS, sender, kFileCreateSuccess,	0, "");
//  return;
//fail:
//  const char *fail = "FAIL";
//  SP_multicast(mbox, SAFE_MESS, sender, kFileCreateFail,	strlen(fail), fail);
//}

//void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
//  if (size<3) goto fail;
//  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
//  if (!RemoveFile(string(msg+1, size-2))) goto fail;

//  SP_multicast(mbox, SAFE_MESS, sender, kFileRemoveSuccess,	0, "");
//  return;
//fail:
//  const char *fail = "FAIL";
//  SP_multicast(mbox, SAFE_MESS, sender, kFileRemoveFail,	strlen(fail), fail);
//}

//void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
//  string data;
//  if (size<3) goto fail;
//  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
//  if (!ReadFile(string(msg+1, size-2), data)) goto fail;

//  SP_multicast(mbox, SAFE_MESS, sender, kFileReadSuccess,	data.size(), data.c_str());
//  return;
//fail:
//  const char *fail = "FAIL";
//  SP_multicast(mbox, SAFE_MESS, sender, kFileReadFail,	strlen(fail), fail);
//}

//void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
//  string data(msg, size);
//  size_t pos = data.size() < 3 ? string::npos : data.find_first_of('"', 2);
//  if (msg[0] != '"' || string::npos == pos) goto fail;

//  if (!WriteFile(data.substr(1,pos-1), data.substr(pos+1))) goto fail;

//  SP_multicast(mbox, SAFE_MESS, sender, kFileWriteSuccess,	data.size(), data.c_str());
//  return;
//fail:
//  const char *fail = "FAIL";
//  SP_multicast(mbox, SAFE_MESS, sender, kFileWriteFail,	strlen(fail), fail);
//}
//const short kSlaveMessage = 0x51A7;
//void SpreadRun() {
//  sp_time timeout{5, 0};
//  mailbox mbox;
//  char group[MAX_PRIVATE_NAME];
//  auto ret = SP_connect_timeout("", NULL, 0, 0, &mbox, group, timeout);
//  if (ACCEPT_SESSION != ret) {
//    cout << "Connection Failure: " << ret << endl;
//    return;
//  }
//  cout << "Connection succeeded. group: " << group << endl;
//  if (!InitStorage(group)) {
//    cout << "Failed initializing storage node " << group << endl;
//  }

//  if (SP_join(mbox, kSlavesGroup)) {
//    cout << "Failed to join " << kSlavesGroup << endl;
//    return;
//  }
//  SP_multicast(mbox, SAFE_MESS, kSlavesGroup, kSlaveMessage, strlen(group), group);

//  char sender[MAX_GROUP_NAME];
//  char groups[32][MAX_GROUP_NAME];
//  int num_groups;
//  short msg_type;
//  vector<char> msg;
//  int endian;

//  while (true) {
//    int sv_type = 0;
//    int ret = SP_receive(mbox, &sv_type, sender, 32, &num_groups,
//                         groups, &msg_type, &endian, msg.size(), msg.data());
//    if (BUFFER_TOO_SHORT == ret) {
//      msg.resize(-endian);
//      continue;
//    }

//    if (ret < 0) {
//      cout << "SP_receive error: " << ret << endl;
//      sleep(1);
//      continue;
//    }
//    cout << msg_type << endl;
//    switch(msg_type) {
//    case kFileCreate:
//      HandleFileOpCreate(mbox, sender, msg.data(), ret);
//      break;
////    case kFileRemove:
////      HandleFileOpRemove(mbox, sender, msg.data(), ret);
////      break;
////    case kFileRead:
////      HandleFileOpRead(mbox, sender, msg.data(), ret);
////      break;
////    case kFileWrite:
////      HandleFileOpWrite(mbox, sender, msg.data(), ret);
////      break;
//    default:
//      cout << "Spurious message received." << endl;
//      break;
//    }
//  }
//  SP_leave(mbox, kSlavesGroup);
//  SP_disconnect(mbox);
//}
