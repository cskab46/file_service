#ifndef SERVER_OPS_H
#define SERVER_OPS_H

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>

#include "utils/ops.h"
#include "utils/file_lock_map.h"

using namespace std;


struct ServerOp {
  FileLockMap file_map;
  vector<pair<string, vector<string>>> slaves;
  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, ServerOp &op, const unsigned int version) {
    ar & op.file_map;
    ar & op.slaves;
  }
};

#endif // SERVER_OPS_H

