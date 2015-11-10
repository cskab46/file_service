#include "file_lock_map.h"

bool FileLockMap::CreateFile(const string &file, const vector<string> &slaves) {
  lock_.lock();
  if (lock_map_.count(file)) {
    lock_.unlock();
    return false;
  }
  lock_map_[file].second = slaves;
  lock_.unlock();
  return true;
}

bool FileLockMap::RemoveFile(const string &file) {
  lock_.lock();
  if (0 == lock_map_.count(file)) {
    lock_.unlock();
    return false;
  }
  lock_map_.erase(file);
  lock_.unlock();
  return true;
}

bool FileLockMap::LockFile(const string &file, vector<string> &slaves) {
  lock_.lock();
  if (0 == lock_map_.count(file)) {
    lock_.unlock();
    return false;
  }
  auto &fl = lock_map_[file];
  lock_.unlock();
  fl.first.lock();
  slaves = fl.second;
  return true;
}

bool FileLockMap::UnlockFile(const string &file) {
  lock_.lock();
  if (0 == lock_map_.count(file)) {
    lock_.unlock();
    return false;
  }
  auto &fl = lock_map_[file];
  lock_.unlock();
  fl.first.unlock();
  return true;
}
