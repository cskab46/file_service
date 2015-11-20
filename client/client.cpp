#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "utils/connection.h"
#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/client_ops.h"

using namespace std;
using namespace std::chrono;

void HandleOp(Connection &con, Message msg) {
  if (!con.SendMessage(msg, kProxyGroup)) {
    cout << "Failed sending file request to server." << endl;
    return;
  }
  auto start = steady_clock::now();
  bool response = false;
  string server;
  ClientOp prepare;
  while (!response &&
         duration_cast<milliseconds>(steady_clock::now() - start).count() < kCreateTimeout) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();
    if (msg.type() == kClientPrepareOp) {
      server = msg.sender();
      msg.GetContent(prepare);
      response = true;
    }
  }
  if (!response) {
    cout << "Server did not respond to request." << endl;
    return;
  }
  if (!con.SendMessage(msg, prepare.slave)) {
    cout << "Failed sending request to slave." << endl;
    return;
  }

  start = steady_clock::now();
  response = false;
  bool succeeded = false;
  while (!response &&
         duration_cast<milliseconds>(steady_clock::now() - start).count() < kCreateTimeout) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();
    if (response = ((msg.type() == kClientConfirmOp || msg.type() == kClientFailOp))) {
       succeeded = msg.type() == kClientConfirmOp;
    }
  }
  if (!response) {
    cout << "Server did not confirm request." << endl;
    return;
  }
  if (succeeded) {
    cout << "Operation done." << endl;
  } else {
    cout << "Operation failed." << endl;
  }
}

void HandleCreateOp(Connection &con, const string &filename,
                    unsigned int redundancy) {
  Message create_msg(kFileCreate, SAFE_MESS);
  CreateFileOp op {filename, redundancy};
  create_msg.SetContent(op);
  HandleOp(con, create_msg);
}

void HandleRemoveOp(Connection &con, const string &filename) {
  Message remove_msg(kFileRemove, SAFE_MESS);
  RemoveFileOp op {filename};
  remove_msg.SetContent(op);
  HandleOp(con, remove_msg);
}

void HandleReadOp(Connection &con, const string &filename) {
  Message read_msg(kFileRead, SAFE_MESS);
  ReadFileOp op {filename};
  read_msg.SetContent(op);
  HandleOp(con, read_msg);
  read_msg.GetContent(op);
  cout << "Data dead: " << op.data << endl;
}

void HandleWriteOp(Connection &con, const string &filename, const string & data) {
  Message write_msg(kFileWrite, SAFE_MESS);
  WriteFileOp op {filename, data};
  write_msg.SetContent(op);
  HandleOp(con, write_msg);
}

int main(int argc, char **argv) {
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
    return 1;
  }
  if (string(argv[1]) == "create" && argc == 4) {
    HandleCreateOp(con, argv[2], stoi(argv[3]));
  } else if (string(argv[1]) == "remove" && argc == 3) {
    HandleRemoveOp(con, argv[2]);
  } else if (string(argv[1]) == "read" && argc == 3) {
    HandleReadOp(con, argv[2]);
  } else if (string(argv[1]) == "write" && argc == 3) {
    HandleWriteOp(con, argv[2], argv[3]);
  } else {
    return 2;
  }
  return 0;
}
