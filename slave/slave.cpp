#include <iostream>
#include <string>
#include <thread>

#include "file_service.h"

using namespace std;

int main(int argc, char **argv) {
  bool quit = false;
  thread file_service(FileService, ref(quit));
  file_service.join();
  return 0;
}
