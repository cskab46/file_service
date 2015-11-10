#ifndef FILE_LOCK_MAP_H
#define FILE_LOCK_MAP_H

#include "utils/ordered_lock.h"
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>

using namespace std;

class FileLockMap {
public:
  bool CreateFile(const string &file, const vector<string> &slaves);
  bool RemoveFile(const string &file);
  bool LockFile(const string &file, vector<string> &slaves);
  bool UnlockFile(const string &file);

  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, FileLockMap &op, const unsigned int version) {
    ar & op.lock_map_;
  }

private:
  ordered_lock lock_;
  map<string, pair<ordered_lock, vector<string>>> lock_map_;
};


#endif // FILE_LOCK_MAP_H
