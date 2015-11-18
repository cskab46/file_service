#ifndef OPS_H_
#define OPS_H_
#include <cstdint>

enum : uint16_t {
  kFileCreate = 2,
  kFileRemove,
  kFileRead,
  kFileWrite,
  kFileOpSuccess,
  kFileOpFail,
  kSlavePrepareOp,
  kSlaveConfirmOp,
  kSlaveFailOp,
  kClientPrepareOp,
  kClientConfirmOp,
  kClientFailOp,
};
#endif // OPS_H_
