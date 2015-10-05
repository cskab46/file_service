#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <streambuf>
#include <sys/stat.h>

using namespace std;

bool Exists(const std::string &filename);
bool CreateFile(const std::string &filename);
bool RemoveFile(const std::string &filename);
bool WriteFile(const std::string &filename, const std::string &data);
bool ReadFile(const std::string &filename, std::string &data);


int main(int argc, char **argv) {

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
  return 0 == stat(filename.c_str(), &buffer);
}

bool CreateFile(const std::string &filename) {
  if (Exists(filename)) return false;
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  return true;
}

bool RemoveFile(const std::string &filename) {
  if (!Exists(filename)) return false;
  return 0 == remove(filename.c_str());
}

bool WriteFile(const std::string &filename, const std::string &data) {
  if (!Exists(filename)) return false;
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  if (!out.good()) return false;
  out << data;
  return true;
}

bool ReadFile(const std::string &filename, std::string &data) {
  if (!Exists(filename)) return false;
  std::ifstream input(filename, std::ios::binary);
  if (!input.good()) return false;
  data = std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
  return true;
}
