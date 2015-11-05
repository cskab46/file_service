#include <iostream>
#include <string>
#include <thread>

#include "server_election.h"

using namespace std;

int main(int argc, char **argv) {
  if (argc < 2) {
    cout << "You have to pass a priority value. Usage: server %priority%" << endl;
    return 0;
  }
  bool lead = false;
  bool quit = false;
  thread leadership(Leadership, argv[1], ref(lead), ref(quit));
  leadership.join();
}
