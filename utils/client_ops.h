#include <cstdint>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>

using namespace std;


enum : uint16_t {
  kClientPrepareOp = 2,
  kClientConfirmOp,
  kClientFailOp,
};

struct ClientOp {
  string file_name;
  string slave;
  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, ClientOp &op, const unsigned int version) {
    ar & op.file_name;
    ar & op.slave;
  }
};
