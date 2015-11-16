#include "file_lock_map.h"

bool FileLockMap::HasFile(const string &file) {
  lock_.lock();
  bool has = 0 != lock_map_.count(file);
  lock_.unlock();
  return has;
}

bool FileLockMap::CreateAndLockFile(const string &file, FileEntry **entry) {
  lock_.lock();
  if (lock_map_.count(file)) {
    lock_.unlock();
    *entry = NULL;
    return false;
  }
  auto &fl = lock_map_[file];
  lock_.unlock();
  fl.lock.lock();
  *entry = &fl;
  return true;
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

bool FileLockMap::GetAndLockFile(const string &file, FileEntry **entry) {
  lock_.lock();
  if (0 == lock_map_.count(file)) {
    lock_.unlock();
    *entry = NULL;
    return false;
  }
  auto &fl = lock_map_[file];
  lock_.unlock();
  fl.lock.lock();
  *entry = &fl;
  return true;
}
