#include "file_lock_map.h"

bool FileLockMap::HasFile(const string &file) {
  lock_.lock();
  bool has = lock_map_.count(file);
  lock_.unlock();
  return has;
}

bool FileLockMap::DestroyFile(const string &file) {
  lock_.lock();
  if (0 == lock_map_.count(file)) {
    lock_.unlock();
    return false;
  }
  lock_map_.erase(file);
  lock_.unlock();
  return true;
}

bool FileLockMap::LockFile(const string &file) {
  lock_.lock();
  auto &fl = lock_map_[file];
  lock_.unlock();
  fl.lock();
  return true;
}

bool FileLockMap::UnlockFile(const string &file) {
  lock_.lock();
  auto &fl = lock_map_[file];
  lock_.unlock();
  fl.unlock();
  return true;
}
