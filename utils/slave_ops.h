#include <cstdint>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>

using namespace std;


enum : uint16_t {
  kSlavePrepareOp = 2,
  kSlaveConfirmOp,
  kSlaveFailOp,
};

struct SlaveOp {
  string file_name;
  string client;
  string handler;
  vector<string> slaves;
  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, SlaveOp &op, const unsigned int version) {
    ar & op.file_name;
    ar & op.client;
    ar & op.handler;
    ar & op.slaves;
  }
};
