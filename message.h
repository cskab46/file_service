#ifndef MESSAGE_H
#define MESSAGE_H

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace boost::archive;

// Bully messages
const short kElectionMessage = 0xDEAD;
const short kAnswerMessage = 0xCAFE;
const short kCoordinatorMessage = 0xBEEF;

// Logging messages
const short kServerLogMessage = 0xD1CA;
const short kClientLogMessage = 0xABCD;

// Slave messages
const short kSlaveMessage = 0x51A7;


const short kCorruptedMessage = 0xFFFF;


class Message {
public:
  Message(int16_t type, int service);
  bool IsMembership(); // Is this a membership message?
  string sender() const;
  void set_sender(const string &sender);
  vector<string> group() const;
  void set_group(const vector<string> &group);
  string data() const;
  void set_data(const string &data);
  int16_t type() const;
  int service() const;
  template <typename T> void SetContent(const T &content);
  template <typename T> void GetContent(T &content);

private:
  string sender_;
  vector<string> group_;
  string data_;
  int16_t type_;
  int service_;
};

template<typename T> void Message::GetContent(T &content) {
  stringstream ss{data_};
  text_iarchive ia(ss);
  ia & content;
}

template<typename T> void Message::SetContent(const T &content) {
  stringstream ss;
  text_oarchive oa{ss};
  oa & content;
  data_ = ss.str();
}

#endif // MESSAGE_H
