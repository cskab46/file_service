#include "message.h"

extern "C" {
  #include <sp.h>
}

Message::Message(uint16_t type, int service)
  : type_(type),
    service_(service) {}

bool Message::IsMembership() {
  return Is_reg_memb_mess(service_);
}

vector<string> Message::group() const {
  return group_;
}

void Message::set_group(const vector<string> &group) {
  group_ = group;
}
string Message::sender() const {
  return sender_;
}

void Message::set_sender(const string &sender){
  sender_ = sender;
}

string Message::data() const {
  return data_;
}

void Message::set_data(const string &data) {
  data_ = data;
}

uint16_t Message::type() const {
  return type_;
}

int Message::service() const {
  return service_;
}
