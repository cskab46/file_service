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
  kFileOpResult,
  kSlavePrepareOp,
  kSlaveConfirmOp,
  kSlaveFailOp,
  kClientPrepareOp,
  kClientConfirmOp,
  kClientFailOp,
  kServerStateOp,
};
#endif // OPS_H_
