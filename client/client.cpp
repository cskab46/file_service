#include <iostream>
#include <string>
#include <thread>

#include "utils/connection.h"
#include "utils/groups.h"
#include "utils/file_ops.h"

using namespace std;

int main(int argc, char **argv) {
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  {
    Message m(kFileRemove, SAFE_MESS);
    RemoveFileOp op {"teste.txt"};
    m.SetContent(op);
    con.SendMessage(m, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));

  {
    Message c(kFileCreate, SAFE_MESS);
    CreateFileOp op {"teste.txt", 10};
    c.SetContent(op);
    con.SendMessage(c, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));

  {
    Message m(kFileRemove, SAFE_MESS);
    RemoveFileOp op {"teste.txt"};
    m.SetContent(op);
    con.SendMessage(m, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));
  {
    Message c(kFileCreate, SAFE_MESS);
    CreateFileOp op {"teste.txt", 10};
    c.SetContent(op);
    con.SendMessage(c, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));
  {
    Message c(kFileCreate, SAFE_MESS);
    CreateFileOp op {"teste.txt", 10};
    c.SetContent(op);
    con.SendMessage(c, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));
  {
    Message m(kFileRead, SAFE_MESS);
    ReadFileOp op {"teste.txt"};
    m.SetContent(op);
    con.SendMessage(m, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));
  {
    Message m(kFileWrite, SAFE_MESS);
    WriteFileOp op {"teste.txt", "13781247128309054!#$@%&$%"};
    m.SetContent(op);
    con.SendMessage(m, kProxyGroup);
  }
  this_thread::sleep_for(chrono::milliseconds(100));
  {
    Message c(kFileCreate, SAFE_MESS);
    CreateFileOp op {"teste.txt", 10};
    c.SetContent(op);
    con.SendMessage(c, kProxyGroup);
  }

  return 0;
}
