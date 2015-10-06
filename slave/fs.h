#include <string>

bool Exists(const std::string &filename);
bool CreateFile(const std::string &filename);
bool RemoveFile(const std::string &filename);
bool WriteFile(const std::string &filename, const std::string &data);
bool ReadFile(const std::string &filename, std::string &data);
