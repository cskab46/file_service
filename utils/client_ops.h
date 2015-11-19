#ifndef CLIENT_OPS_H
#define CLIENT_OPS_H
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>

#include "utils/ops.h"

using namespace std;


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

#endif // CLIENT_OPS_H
