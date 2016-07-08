#include "StringList.h"
#include "path.h"
#include <map>
#include <vector>

Dictionary readStrings(char const* name, SnoLoader* loader) {
  Dictionary res;
  SnoFile<StringList> list(name, loader);
  for (auto& item : list->x10_StringTableEntries) {
    res.emplace(item.x00_Text.text(), item.x10_Text.text());
  }
  return res;
}
