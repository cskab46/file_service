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


void HandleCreateOp(Connection &con, const string &filename,
                    unsigned int redundancy) {
  Message create_msg(kFileCreate, SAFE_MESS);
  CreateFileOp op {filename, redundancy};
  create_msg.SetContent(op);
  if (!con.SendMessage(create_msg, kProxyGroup)) {
    cout << "Failed sending create file request to server." << endl;
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
    cout << "Server did not respond to create file request." << endl;
    return;
  }
  if (!con.SendMessage(create_msg, prepare.slave)) {
    cout << "Failed sending create file request to slave." << endl;
    return;
  }

  start = steady_clock::now();
  response = false;
  while (!response &&
         duration_cast<milliseconds>(steady_clock::now() - start).count() < kCreateTimeout) {
    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();
    if (msg.type() == kClientConfirmOp && msg.sender() == server) {
      response = true;
    }
  }
  if (!response) {
    cout << "Server did not confirm create file request." << endl;
    return;
  }
  cout << "File created successfully." << endl;
}

int main(int argc, char **argv) {
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
    return 1;
  }
  HandleCreateOp(con, "teste.txt", 4);
  system("pause");
//  {
//    Message m(kFileRemove, SAFE_MESS);
//    RemoveFileOp op {"teste.txt"};
//    m.SetContent(op);
//    con.SendMessage(m, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));

//  {
//    Message c(kFileCreate, SAFE_MESS);
//    CreateFileOp op {"teste.txt", 10};
//    c.SetContent(op);
//    con.SendMessage(c, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));

//  {
//    Message m(kFileRemove, SAFE_MESS);
//    RemoveFileOp op {"teste.txt"};
//    m.SetContent(op);
//    con.SendMessage(m, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));
//  {
//    Message c(kFileCreate, SAFE_MESS);
//    CreateFileOp op {"teste.txt", 10};
//    c.SetContent(op);
//    con.SendMessage(c, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));
//  {
//    Message c(kFileCreate, SAFE_MESS);
//    CreateFileOp op {"teste.txt", 10};
//    c.SetContent(op);
//    con.SendMessage(c, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));
//  {
//    Message m(kFileRead, SAFE_MESS);
//    ReadFileOp op {"teste.txt"};
//    m.SetContent(op);
//    con.SendMessage(m, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));
//  {
//    Message m(kFileWrite, SAFE_MESS);
//    WriteFileOp op {"teste.txt", "13781247128309054!#$@%&$%"};
//    m.SetContent(op);
//    con.SendMessage(m, kProxyGroup);
//  }
//  this_thread::sleep_for(chrono::milliseconds(100));
//  {
//    Message c(kFileCreate, SAFE_MESS);
//    CreateFileOp op {"teste.txt", 10};
//    c.SetContent(op);
//    con.SendMessage(c, kProxyGroup);
//  }

  return 0;
}
