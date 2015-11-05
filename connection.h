#ifndef CONNECTION_H
#define CONNECTION_H
#include <string>

extern "C" {
  #include <sp.h>
  #include <unistd.h>
}
#include "message.h"

using namespace std;

class Connection {
public:
  static Connection Connect(bool &err, bool membership_msgs = false);
  static Connection Connect(const string &name, bool &err,
                            bool membership_msgs = false);
  static Connection Connect(const string &server, const string &name, bool &err,
                            bool membership_msgs = false);
  virtual ~Connection();
  string identifier() const;
  bool JoinGroup(const string &group);
  bool HasMessage();
  const Message GetMessage();
  bool SendMessage(const Message &msg, const string &to);

private:
  Connection(const mailbox &mbox, const string &identifier);
  mailbox mbox_;
  string identifier_;
};

#endif // CONNECTION_H
