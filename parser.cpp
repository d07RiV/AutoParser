#include "parser.h"
#include "snomap.h"
#include <algorithm>

uint32 HashName(std::string const& str) {
  uint32 hash = 0;
  for (uint8 c : str) {
    hash = hash * 33 + c;
  }
  return hash;
}
uint32 HashNameLower(std::string const& str) {
  uint32 hash = 0;
  for (uint8 c : str) {
    hash = hash * 33 + std::tolower(c);
  }
  return hash;
}

#ifdef _MSC_VER
__declspec(thread) SnoParser* SnoParser::context = nullptr;

#define NOMINMAX
#include <windows.h>
#else
__thread SnoParser* SnoParser::context = nullptr;

#include <dirent.h>
#endif

SnoSysLoader::SnoSysLoader(std::string dir)
  : dir_(dir)
{
  if (dir.empty()) {
    dir_ = path::root();
  }
}

std::vector<std::string> SnoSysLoader::listdir(SnoInfo const& type) {
  std::vector<std::string> list;
#ifdef _MSC_VER
  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile((dir_ / type.type / "*" + type.ext).c_str(), &data);
  if (hFind == INVALID_HANDLE_VALUE) return list;
  do {
    if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, "..")) continue;
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
    list.push_back(path::title(data.cFileName));
  } while (FindNextFile(hFind, &data));
  FindClose(hFind);
#else
  struct dirent* ent;
  DIR* dir = opendir((dir_ / type.type).c_str());
  if (!dir) return list;
  while ((ent = readdir(dir))) {
    if (ent->d_type & DT_DIR) {
      continue;
    }
    if (strlower(path::ext(ent->d_name)) == type.ext) {
      list.push_back(path::title(ent->d_name));
    }
  }
  closedir(dir);
#endif
  return list;
}
File SnoSysLoader::loadfile(SnoInfo const& type, char const* name) {
  return File(dir_ / type.type / name + type.ext, "rb");
}

SnoLoader* SnoLoader::primary = nullptr;
