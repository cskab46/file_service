#ifndef SERVER_ELECTION_H
#define SERVER_ELECTION_H

#include <string>
using namespace std;

//!< Handles the leadership task, notifying of leader changes. Exists when quit is true.
void Leadership(const string &priority, bool &lead, const bool &quit);

#endif // SERVER_ELECTION_H
