#include <string>
#include <vector>
#include <sstream>
using namespace std;

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

const char kDelimiter = '#';

class Arguments {
public:
  Arguments(const string &data) : data_(data) {}
  Arguments& PushArg(const string &arg) {
    stringstream ss;
    ss << kDelimiter << arg.size() << kDelimiter << data;
    data_.append(ss.str());
    return *this;
  }

  string data() const {
    return data_;
  }
  bool Deserialize(vector<string> &args) {
    auto str = data_.data();
    for (int i = 0; i + 1 < data_.size() && data_[i] == kDelimiter;) {
      try {
        i++;
        auto number_size = 0;
        auto arg_size = stoi(str[i], &number_size);
        i += number_size;
        if (str[i] != kDelimiter) return false;
        i++;
        if (i + arg_size >= data_.size()) return false;
        args.push_back(data_.substr(i, arg_size));
        i += arg_size;
      } catch(...) {
        return false;
      }
    }
    return true;
  }

private:
  string data_;
};
