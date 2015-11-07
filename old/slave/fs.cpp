#include "fs.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
using namespace std;

string gFolderName;

bool InitStorage(string volume) {
  struct stat st;
  if (stat(volume.c_str(), &st) == 0) {
    return false;
  }
  if (mkdir(volume.c_str(), 0700)) {
      return false;
  }
  gFolderName = volume + "/";
  return true;
}

std::vector<std::string> gFiles;

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

bool Exists(std::string filename) {
  struct stat buffer;
  cout << "Exists: " << stat(filename.c_str(), &buffer) << endl;
  cout << errno << endl;
  return 0 == stat(filename.c_str(), &buffer);
}

bool CreateFile(std::string filename) {
  filename = gFolderName + filename;
  if (Exists(filename)) return false;
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  gFiles.push_back(filename);
  return true;
}

bool RemoveFile(std::string filename) {
  filename = gFolderName + filename;
  if (!Exists(filename)) return false;
  cout << "Trying to remove file: " << filename << endl;
  auto res = 0 == remove(filename.c_str());
  cout  << "Result: " << res << endl;
  return res;
}

bool WriteFile(std::string filename, const std::string &data) {
  filename = gFolderName + filename;
  if (!Exists(filename)) return false;
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  if (!out.good()) return false;
  out << data;
  return true;
}

bool ReadFile(std::string filename, std::string &data) {
  filename = gFolderName + filename;
  cout << "File to be read: " << filename;
  if (!Exists(filename)) return false;
  std::ifstream input(filename, std::ios::binary);
  if (!input.good()) {
    cout << "Input not good." << endl;
    return false;
  }
  data = std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
  return true;
}
