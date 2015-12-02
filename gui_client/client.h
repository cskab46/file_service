#ifndef CLIENT_H
#define CLIENT_H

#include <string>

using namespace std;

class Connection;

bool HandleCreateOp(Connection &con, const string &filename,
                    unsigned int redundancy);

bool HandleRemoveOp(Connection &con, const string &filename);

bool HandleReadOp(Connection &con, const string &filename, string &data);

bool HandleWriteOp(Connection &con, const string &filename, const string & data);

#endif // CLIENT_H

