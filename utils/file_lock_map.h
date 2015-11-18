#ifndef FILE_LOCK_MAP_H
#define FILE_LOCK_MAP_H

#include "utils/ordered_lock.h"
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>

using namespace std;

struct FileEntry {
  ordered_lock lock;
  vector<string> slaves;
};

class FileLockMap {
public:
  bool HasFile(const string &file);
  bool CreateAndLockFile(const string &file, FileEntry **entry);
  bool LockFile(const string &file);
  bool UnlockFile(const string &file);
  bool DestroyFile(const string &file);

  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, FileLockMap &op, const unsigned int version) {
    ar & op.lock_map_;
  }

private:
  ordered_lock lock_;
  map<string, FileEntry> lock_map_;
};


#endif // FILE_LOCK_MAP_H
