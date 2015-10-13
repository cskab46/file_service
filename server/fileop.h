#include <cstdint>

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
