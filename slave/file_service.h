#include <iostream>
#include <vector>
#include <cstring>
#include <thread>

#include "fs.h"
#include "utils/groups.h"
#include "utils/slave_ops.h"
#include "utils/message.h"
#include "utils/connection.h"

using namespace std;

void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size);
void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);

void FileService(const bool &quit);

void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size) {
  if (size<3) goto fail;
  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
  if (!CreateFile(string(msg+1, size-2))) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileCreateSuccess,	0, "");
  return;
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileCreateFail,	strlen(fail), fail);
}

void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
  if (size<3) goto fail;
  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
  if (!RemoveFile(string(msg+1, size-2))) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileRemoveSuccess,	0, "");
  return;
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileRemoveFail,	strlen(fail), fail);
}

void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
  string data;
  if (size<3) goto fail;
  if (msg[0] != '"' || msg[size-1] != '"') goto fail;
  if (!ReadFile(string(msg+1, size-2), data)) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileReadSuccess,	data.size(), data.c_str());
  return;
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileReadFail,	strlen(fail), fail);
}

void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size) {
  string data(msg, size);
  size_t pos = data.size() < 3 ? string::npos : data.find_first_of('"', 2);
  if (msg[0] != '"' || string::npos == pos) goto fail;

  if (!WriteFile(data.substr(1,pos-1), data.substr(pos+1))) goto fail;

  SP_multicast(mbox, SAFE_MESS, sender, kFileWriteSuccess,	data.size(), data.c_str());
  return;
fail:
  const char *fail = "FAIL";
  SP_multicast(mbox, SAFE_MESS, sender, kFileWriteFail,	strlen(fail), fail);
}

void HandleOperation(const string &my_id, const Message &msg) {
  try {
    bool err;
    auto con = Connection::Connect(err);
    if (err) {
      cout << "Failed to connect with spread daemon." << endl;
    }
    SlaveOp op;
    msg.GetContent(op);
    // Handle the prepare and the operation itself
    while (true) {
      if (!con.HasMessage()) continue;
      auto msg = con.GetMessage();

      switch (msg.type()) {
      case kSlavePrepareOp:
      case kSlaveConfirmOp:
      case kSlaveFailOp:

      }

    }
  }


}

void FileService(const bool &quit) {
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  if (!con.JoinGroup(kSlavesGroup)) {
    cout << "Failed to join group: " << kSlavesGroup << endl;
    return;
  }

  if (!InitStorage(con.identifier())) {
    cout << "Failed initializing storage node " << con.identifier() << endl;
  }

  while (!quit) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();

    if (msg.type() == kSlavePrepareOp) {
      thread t(HandleOperation, ref(con.identifier()), ref(msg));
      t.detach();
    } else {
      cout << "Spurious slave op requested." << endl;
    }
  }
//    cout << msg_type << endl;
//    switch(msg_type) {
//    case kFileCreate:
//      HandleFileOpCreate(mbox, sender, msg.data(), ret);
//      break;
//    case kFileRemove:
//      HandleFileOpRemove(mbox, sender, msg.data(), ret);
//      break;
//    case kFileRead:
//      HandleFileOpRead(mbox, sender, msg.data(), ret);
//      break;
//    case kFileWrite:
//      HandleFileOpWrite(mbox, sender, msg.data(), ret);
//      break;
//    default:
//      cout << "Spurious message received." << endl;
//      break;
//    }
}
