#ifndef CONNECTION_H
#define CONNECTION_H
#include <string>

extern "C" {
  #include <sp.h>
  #include <unistd.h>
}
#include "message.h"

using namespace std;

class Connection
{
public:
  static bool Connect(bool membership_msgs, Connection &con);
  virtual ~Connection();
  string identifier() const;
  bool HasMessage();
  const Message GetMessage();
  bool SendMessage(const Message &msg, const string &to);

private:
  Connection(const mailbox &mbox, const string &identifier);
  mailbox mbox_;
  string identifier_;
};

#endif // CONNECTION_H
