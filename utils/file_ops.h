#include <cstdint>
#include <string>
#include <boost/serialization/access.hpp>

enum : uint16_t {
  kFileCreate = 2,
  kFileRemove,
  kFileRead,
  kFileWrite,
  kFileCreateSuccess,
  kFileCreateFail,
  kFileRemoveSuccess,
  kFileRemoveFail,
  kFileReadSuccess,
  kFileReadFail,
  kFileWriteSuccess,
  kFileWriteFail
};

struct CreateFileOp {
  std::string file_name;
  unsigned int redundancy;
  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, CreateFileOp &op, const unsigned int version) {
    ar & op.file_name;
    ar & op.redundancy;
  }
};

struct RemoveFileOp {
  std::string file_name;
  friend class boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, RemoveFileOp &op, const unsigned int version) {
    ar & op.file_name;
  }
};

struct ReadFileOp {
  std::string file_name;
  friend struct boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, ReadFileOp &op, const unsigned int version) {
    ar & op.file_name;
  }
};

struct WriteFileOp {
  std::string file_name;
  std::string data;
  friend struct boost::serialization::access;
  template <typename Archive>
  friend void serialize(Archive &ar, WriteFileOp &op, const unsigned int version) {
    ar & op.file_name;
    ar & op.data;
  }
};
