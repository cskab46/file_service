#include <iostream>
#include <vector>
#include <cstring>
#include <thread>

#include "fs.h"
#include "utils/groups.h"
#include "utils/slave_ops.h"
#include "utils/file_ops.h"
#include "utils/message.h"
#include "utils/connection.h"

using namespace std;

//void HandleFileOpCreate(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, const size_t size);
//void HandleFileOpRemove(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
//void HandleFileOpRead(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);
//void HandleFileOpWrite(mailbox &mbox, const char sender[MAX_GROUP_NAME], const char *msg, size_t size);

void FileService(const bool &quit);

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

bool HandleFileCreate(const Message &op_msg) {
  try {
    CreateFileOp fop;
    op_msg.GetContent(fop);
    if (!CreateFile(fop.file_name)) {
      cout << "Failed to create file: " << fop.file_name << endl;
      return false;
    }
    return true;
  } catch (...) {
    cout << "Exception during HandleFileCreate." << endl;
  }
  return false;
}

#include <chrono>

using namespace std::chrono;

const int kHandleOpTimeout = 3000;

void HandleOperation(const Message &msg) {
  if (kSlavePrepareOp != msg.type()) {
    cout << "Invalid operation to handle. Type: " << msg.type() << endl;
  }
  try {
    bool err;
    auto con = Connection::Connect(err);
    if (err) {
      cout << "Failed to connect with spread daemon." << endl;
    }
    SlaveOp op;
    msg.GetContent(op);
    op.handler = con.identifier();
    Message response(kSlaveConfirmOp, SAFE_MESS);
    response.SetContent(op);
    if (!con.SendMessage(response, msg.sender())) {
      cout << "Failed to confirm. Aborting the send." << endl;
      return;
    }

    // Waiting for the client request
    auto start = steady_clock::now();
    bool requested = false;
    Message req(0,0);
    while (duration_cast<milliseconds>(steady_clock::now() - start).count() < kHandleOpTimeout) {
      if (!con.HasMessage()) continue;
      req = con.GetMessage();
      if (req.sender() != op.client) continue;
      requested = true;
      break;
    }
    for (auto &slave : op.slaves) {
      if (!con.SendMessage(req, slave)) {
        cout << "Failed to request the operation to slave: " << slave << endl;
        break;
      }
      // Waiting for slave confirmation
      auto start = steady_clock::now();
      bool confirmed = false;
      while (duration_cast<milliseconds>(steady_clock::now() - start).count() < kHandleOpTimeout) {
        if (!con.HasMessage()) continue;
        auto req = con.GetMessage();
        if (req.sender() != slave) continue;
        if (req.type() == kFileOpFail) break;
        if (req.type() == kFileOpSuccess) {
          confirmed = true;
          break;
        }
      }
      // TODO: handle revert.
      if (!confirmed) {
        cout << "Could not finish the operation." << endl;
        return;
      }
    }

    if (!con.SendMessage(response, msg.sender())) {
      cout << "Failed to confirm. Aborting the send." << endl;
      return;
    }
  } catch(...) {
    cout << "Exception thrown while handling operation." << endl;
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
      thread t(HandleOperation, ref(msg));
      t.detach();
      continue;
    }
    bool handled = false;
    if (msg.type() == kFileCreate) {
      handled = HandleFileCreate(msg);
    } else if (msg.type() == kFileRemove) {
      handled = false;
     } else {
      cout << "Spurious slave op requested." << endl;
      continue;
    }
    Message response(handled ? kFileOpSuccess : kFileOpFail, SAFE_MESS);
    if (con.SendMessage(response, msg.sender())) {
      cout << "Could not confirm FileOp." << endl;
    }
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
//}
