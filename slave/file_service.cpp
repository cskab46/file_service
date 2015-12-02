#include "file_service.h"

#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>

#include "fs.h"
#include "utils/groups.h"
#include "utils/slave_ops.h"

using namespace std;
using namespace std::chrono;

const int kHandleOpTimeout = 3000;

ResultFileOp HandleFileCreate(const Message &op_msg) {
  try {
    CreateFileOp fop;
    op_msg.GetContent(fop);
    if (!CreateFile(fop.file_name)) {
      cout << "Failed to create file: " << fop.file_name << endl;
      return ResultFileOp{fop.file_name, false};
    }
    return ResultFileOp{fop.file_name, true};
  } catch (...) {
    cout << "Exception during HandleFileCreate." << endl;
  }
  return ResultFileOp{"", false};
}


ResultFileOp HandleFileRemove(const Message &op_msg) {
  try {
    RemoveFileOp fop;
    op_msg.GetContent(fop);
    if (!RemoveFile(fop.file_name)) {
      cout << "Failed to remove file: " << fop.file_name << endl;
      return ResultFileOp{fop.file_name, false};
    }
    return ResultFileOp{fop.file_name, true};
  } catch (...) {
    cout << "Exception during HandleFileRemove." << endl;
  }
  return ResultFileOp{"", false};
}


ResultFileOp HandleFileRead(const Message &op_msg) {
  try {
    ReadFileOp fop;
    op_msg.GetContent(fop);
    string data;
    if (!ReadFile(fop.file_name, data)) {
      cout << "Failed to read file: " << fop.file_name << endl;
      return ResultFileOp{fop.file_name, false};
    }
    return ResultFileOp{fop.file_name, true, data};
  } catch (...) {
    cout << "Exception during HandleFileRemove." << endl;
  }
  return ResultFileOp{"", false};
}


ResultFileOp HandleFileWrite(const Message &op_msg) {
  try {
    WriteFileOp fop;
    op_msg.GetContent(fop);
    if (!WriteFile(fop.file_name, fop.data)) {
      cout << "Failed to write file: " << fop.file_name << endl;
      return ResultFileOp{fop.file_name, false};
    }
    return ResultFileOp{fop.file_name, true};
  } catch (...) {
    cout << "Exception during HandleFileRemove." << endl;
  }
  return ResultFileOp{"", false};
}


void HandleOperation(const Message &msg) {
  if (kSlavePrepareOp != msg.type()) {
    cout << "Invalid operation to handle. Type: " << msg.type() << endl;
    return;
  }
  try {
    bool err;
    auto con = Connection::Connect(err);
    if (err) {
      cout << "Failed to connect with spread daemon." << endl;
    }
    SlaveOp op;
    msg.GetContent(op);
    Message response(kSlaveConfirmOp, SAFE_MESS);
    response.SetContent(SlaveOp{op.file_name, op.client, con.identifier(), vector<string>()});
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
    if (!requested) {
      cout << "Client request did not arrive. Handler will be terminated." << endl;
      return;
    }
    bool slaves_failed = false;
    Message result(0,0);
    for (auto &slave : op.slaves) {
      if (!con.SendMessage(req, slave)) {
        cout << "Failed to request the operation to slave: " << slave << endl;
        cout << "Aborting operation." << endl;
        return;
      }
      if (!con.GetMessage(kFileOpResult, kHandleOpTimeout, result)) {
        slaves_failed = true;
        break;
      }
      cout << "slave " << slave << " confirmed op." << endl;
      cout << "msg: " << result.type() << endl;
      ResultFileOp rop;
      result.GetContent(rop);
      cout << "msg: " << rop.file_name << endl;
      cout << "msg: " << rop.ok << endl;
      cout << "msg: " << rop.data << endl;
    }
    if (slaves_failed) {
      cout << "Some of the slaves failed. This should not happen." << endl;
      ResultFileOp res{op.file_name, false};
      result.SetContent(res);
      if (!con.SendMessage(result, op.client) ||
          !con.SendMessage(result, msg.sender())) {
        cout << "Failed to inform client and server of slaves failure." << endl;
      }
    } else {
      if (!con.SendMessage(result, op.client) ||
          !con.SendMessage(result, msg.sender())) {
        cout << "Failed to inform client and server of operation result." << endl;
      }
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
      thread t(HandleOperation, msg);
      t.detach();
      continue;
    }
    ResultFileOp result = {"", false};
    switch (msg.type()) {
    case kFileCreate:
      result = HandleFileCreate(msg);
      break;
    case kFileRemove:
      result = HandleFileRemove(msg);
      break;
    case kFileRead:
      result = HandleFileRead(msg);
      break;
    case kFileWrite:
      result = HandleFileWrite(msg);
      break;
    default:
      cout << "Spurious slave op requested." << endl;
      continue;
    }

    Message response(kFileOpResult, SAFE_MESS);
    response.SetContent(result);
    if (!con.SendMessage(response, msg.sender())) {
      cout << "Could not confirm FileOp." << endl;
    }
  }
}
