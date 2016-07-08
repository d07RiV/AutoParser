#include "path.h"
#include "file.h"
#include "common.h"
#include <sys/stat.h>
using namespace std;

string path::name(string const& path) {
  int pos = path.length();
  while (pos > 0 && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
  }
  return path.substr(pos);
}
string path::title(string const& path) {
  size_t pos = path.length();
  size_t dot = path.length();
  while (pos && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
    if (path[pos] == '.' && dot == path.length()) {
      dot = pos;
    }
  }
  if (dot == pos) {
    return path.substr(pos);
  } else {
    return path.substr(pos, dot - pos);
  }
}
string path::path(string const& path) {
  int pos = path.length();
  while (pos > 0 && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
  }
  return path.substr(0, pos ? pos - 1 : 0);
}
string path::ext(string const& path) {
  size_t pos = path.length();
  size_t dot = path.length();
  while (pos && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
    if (path[pos] == '.' && dot == path.length()) {
      dot = pos;
    }
  }
  if (dot == pos) {
    return "";
  } else {
    return path.substr(dot);
  }
}

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace path {
  struct Paths {
    string root, work;
    static Paths& get();
  };
  bool initialized = false;

  extern std::vector<std::string> roots;

  Paths& Paths::get() {
    static bool initialized = false;
    static Paths instance;
    if (initialized) return instance;
    for (auto const& root : roots) {
#ifdef _MSC_VER
      uint32 attr = GetFileAttributes(root.c_str());
      if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
#else
      struct stat sb;
      if (stat(root.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
#endif
        instance.root = root;
        initialized = true;
        break;
      }
    }
    if (!initialized) throw Exception("root dir not found");

    char buffer[512];
#ifdef _MSC_VER
    GetModuleFileName(GetModuleHandle(NULL), buffer, sizeof buffer);
    instance.work = path(buffer) / "Work";
    SetCurrentDirectory(instance.work.c_str());
#else
    readlink("/proc/self/exe", buffer, sizeof buffer);
    instance.work = path(buffer) / "Work";
    chdir(instance.work.c_str());
#endif

    return instance;
  }
}

std::string const& path::root() {
  return Paths::get().root;
}
std::string const& path::work() {
  return Paths::get().work;
}

string operator / (string const& lhs, string const& rhs) {
  if (lhs.empty() || rhs.empty()) return lhs + rhs;
  bool left = (lhs.back() == '\\' || lhs.back() == '/');
  bool right = (rhs.front() == '\\' || rhs.front() == '/');
  if (left && right) {
    string res = lhs;
    res.pop_back();
    return res + rhs;
  } else if (!left && !right) {
    return lhs + path::sep + rhs;
  } else {
    return lhs + rhs;
  }
}
