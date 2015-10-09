#include "fs.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
using namespace std;

std::vector<std::string> gFiles;
bool HasFile(const std::string &file) {
  bool has = gFiles.end() != find(begin(gFiles), end(gFiles), file);
  cout << "HasFile: " << has << endl;
  return has;
}

int test() {

  std::string data;
  cout << Exists("teste.txt") << endl;
  cout << RemoveFile("teste.txt") << endl;
  cout << CreateFile("teste.txt") << endl;
  cout << Exists("teste.txt") << endl;
  cout << RemoveFile("teste.txt") << endl;
  cout << WriteFile("teste.txt", "teste") << endl;

  cout << ReadFile("teste.txt", data) << endl;
  cout << data << endl;


  cout << CreateFile("teste.txt") << endl;
  cout << WriteFile("teste.txt", "teste") << endl;
  cout << ReadFile("teste.txt", data) << endl;
  cout << data << endl;
  return 0;
}

bool Exists(const std::string &filename) {
  struct stat buffer;
  cout << "Exists: " << stat(filename.c_str(), &buffer) << endl;
  cout << errno << endl;
  return 0 == stat(filename.c_str(), &buffer);
}

bool CreateFile(const std::string &filename) {
  if (Exists(filename)) return false;
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  gFiles.push_back(filename);
  return true;
}

bool RemoveFile(const std::string &filename) {
  if (!Exists(filename)) return false;
  if (!HasFile(filename)) return false;
  return 0 == remove(filename.c_str());
}

bool WriteFile(const std::string &filename, const std::string &data) {
  if (!Exists(filename)) return false;
  if (!HasFile(filename)) return false;
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  if (!out.good()) return false;
  out << data;
  return true;
}

bool ReadFile(const std::string &filename, std::string &data) {
  cout << "File to be read: " << filename;
  if (!Exists(filename)) return false;
  if (!HasFile(filename)) return false;
  std::ifstream input(filename, std::ios::binary);
  if (!input.good()) {
    cout << "Input not good." << endl;
    return false;
  }
  data = std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
  return true;
}
