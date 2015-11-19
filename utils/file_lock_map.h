#ifndef FILE_LOCK_MAP_H
#define FILE_LOCK_MAP_H

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>

#include "utils/ordered_lock.h"

using namespace std;

class FileLockMap {
public:
  bool HasFile(const string &file);
  bool LockFile(const string &file);
  bool UnlockFile(const string &file);
  bool DestroyFile(const string &file);
  void operator =(const FileLockMap &other) {
    lock_.lock();
    this->lock_map_ = other.lock_map_;
    lock_.unlock();
  }

  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, FileLockMap &op, const unsigned int version) {
    ar & op.lock_map_;
  }

private:
  ordered_lock lock_;
  map<string, ordered_lock> lock_map_;
};


#endif // FILE_LOCK_MAP_H
