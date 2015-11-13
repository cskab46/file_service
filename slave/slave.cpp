#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main(int argc, char **argv) {
  bool quit = false;
  thread leadership(Leadership, argv[1], ref(lead), ref(quit));
  thread proxy(Proxy, ref(lead), ref(quit));
  leadership.join();
  proxy.join();
  return 0;
}
