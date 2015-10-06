#include <iostream>
#include <vector>
#include <cstring>

extern "C" {
  #include <sp.h>
  #include <unistd.h>
}

#include "fs.h"
#include "fileop.h"

using namespace std;

const char *kGroup = "SLAVES";

void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size);
void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);

void SpreadRun();

int main(int argc, char **argv) {
  SpreadRun();
  return 0;
}

void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size) {
  if (!size) goto fail;
  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
  if (!CreateFile(string(msg, size))) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileCreateSuccess,	0, "");
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileCreateFail,	strlen(fail), fail);
}

void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
  if (!size) goto fail;
  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
  if (!RemoveFile(string(msg, size))) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileRemoveSuccess,	0, "");
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileRemoveFail,	strlen(fail), fail);
}

void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
  string data;
  if (!size) goto fail;
  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
  if (!ReadFile(string(msg, size), data)) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileReadSuccess,	data.size(), data.c_str());
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileReadFail,	strlen(fail), fail);
}

void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
  if (size < 3) goto fail;
  string data(msg, size);
  size_t pos = data.find_first_of('"', 2);
  if (msh[0] != '"' || string::npos == pos) goto fail;

  if (!WriteFile(data.substr(1,pos-1), data.substr(pos+1))) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileWriteSuccess,	data.size(), data.c_str());
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileWriteFail,	strlen(fail), fail);
}

void SpreadRun() {
  sp_time timeout{5, 0};
  mailbox mbox;
  char group[MAX_PRIVATE_NAME];
  auto ret = SP_connect_timeout("", NULL, 0, 1, &mbox, group, timeout);
  if (ACCEPT_SESSION != ret) {
    cout << "Connection Failure: " << ret << endl;
    return;
  }

  if (SP_join(mbox, kGroup)) {
    cout << "Failed to join " << kGroup << endl;
    return;
  }

  char sender[MAX_GROUP_NAME];
  char groups[32][MAX_GROUP_NAME];
  int num_groups;
  short msg_type;
  vector<char> msg;
  int endian;

  while (true) {
    int sv_type = 0;
    int ret = SP_receive(mbox, &sv_type, sender, 32, &num_groups,
                         groups, &msg_type, &endian, msg.size(), msg.data());
    if (BUFFER_TOO_SHORT == ret) {
      msg.reserve(endian);
      continue;
    }

    if (ret < 0) {
      cout << "SP_receive error: " << ret << endl;
      sleep(1);
      continue;
    }
    if (Is_reg_memb_mess(sv_type)) {
      continue;
    }

    switch(msg_type) {
    case kFileOpCreate:
      HandleFileOpCreate(mbox, sender, msg.data(), ret);
      break;
    case kFileOpRemove:
      HandleFileOpRemove(mbox, sender, msg.data(), ret);
      break;
    case kFileOpRead:
      HandleFileOpRead(mbox, sender, msg.data(), ret);
      break;
    case kFileOpWrite:
      HandleFileOpWrite(mbox, sender, msg.data(), ret);
      break;
    default:
      cout << "Spurious message received." << endl;
      break;
    }
  }
  SP_leave(mbox, kGroup);
  SP_disconnect(mbox);
}
