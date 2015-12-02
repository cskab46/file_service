#include "client.h"

//#include <iostream>
#include <chrono>

#include "utils/connection.h"
#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/client_ops.h"

using namespace std::chrono;

const int kHandleOpTimeout = 3000;

bool HandleOp(Connection &con, Message msg, ResultFileOp &result) {
  if (!con.SendMessage(msg, kProxyGroup)) {
    //cout << "Failed sending file request to server." << endl;
    return false;
  }
  Message prep_msg(0,0);
  if (!con.GetMessage(kClientPrepareOp, kHandleOpTimeout, prep_msg)) {
    //cout << "Server did not respond to request." << endl;
    return false;
  }

  string slave;
  try {
    ClientOp prepare;
    prep_msg.GetContent(prepare);
    slave = prepare.slave;
  } catch (...) {
    //cout << "Failed to get result information from messages." << endl;
    return false;
  }

  if (!con.SendMessage(msg, slave)) {
    //cout << "Failed sending request to slave." << endl;
    return false;
  }
  // Wait for slave response
  Message slave_response(0,0);
  if (!con.GetMessage(kFileOpResult, kHandleOpTimeout, slave_response)) {
    //cout << "Slave did not respond to request." << endl;
    return false;
  }
  // Wait for server response
  Message server_response(0,0);
  if (!con.GetMessage(kFileOpResult, kHandleOpTimeout, server_response)) {
    //cout << "Server did not respond to request." << endl;
    return false;
  }
  ResultFileOp slave_result, server_result;
  try {
    slave_response.GetContent(slave_result);
    server_response.GetContent(server_result);
  } catch (...) {
    //cout << "Failed to get result information from messages." << endl;
    return false;
  }

  if (slave_result.ok && server_result.ok) {
    result = slave_result;
    //cout << "Operation done." << endl;
    return true;
  }

  //cout << "Operation failed." << endl;
  return false;
}

bool HandleCreateOp(Connection &con, const string &filename,
                    unsigned int redundancy) {
  Message create_msg(kFileCreate, SAFE_MESS);
  CreateFileOp op {filename, redundancy};
  create_msg.SetContent(op);
  ResultFileOp result;
  return HandleOp(con, create_msg, result);
}

bool HandleRemoveOp(Connection &con, const string &filename) {
  Message remove_msg(kFileRemove, SAFE_MESS);
  RemoveFileOp op {filename};
  remove_msg.SetContent(op);
  ResultFileOp result;
  return HandleOp(con, remove_msg, result);
}

bool HandleReadOp(Connection &con, const string &filename, string &data) {
  Message read_msg(kFileRead, SAFE_MESS);
  ReadFileOp op {filename};
  read_msg.SetContent(op);
  ResultFileOp result;
  if (!HandleOp(con, read_msg, result)) {
    return false;
  }
  data = result.data;
  return true;
}

bool HandleWriteOp(Connection &con, const string &filename, const string & data) {
  Message write_msg(kFileWrite, SAFE_MESS);
  WriteFileOp op {filename, data};
  write_msg.SetContent(op);
  ResultFileOp result;
  return HandleOp(con, write_msg, result);
}
