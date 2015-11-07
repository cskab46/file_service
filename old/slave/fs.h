#include <string>

bool InitStorage(std::string volume);
bool Exists(std::string filename);
bool CreateFile(std::string filename);
bool RemoveFile(std::string filename);
bool WriteFile(std::string filename, const std::string &data);
bool ReadFile(std::string filename, std::string &data);
