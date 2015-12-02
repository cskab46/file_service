#include "utils/file_ops.h"
#include "utils/message.h"
#include "utils/connection.h"


ResultFileOp HandleFileCreate(const Message &op_msg);
ResultFileOp HandleFileRemove(const Message &op_msg);
ResultFileOp HandleFileRead(const Message &op_msg);
ResultFileOp HandleFileWrite(const Message &op_msg);
void HandleOperation(const Message &msg);

void FileService(const bool &quit);
