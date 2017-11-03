#pragma once

#include <string>
#include <vector>

namespace path {
  std::string name(std::string const& path);
  std::string title(std::string const& path);
  std::string path(std::string const& path);
  std::string ext(std::string const& path);

  std::vector<std::string> listdir(std::string const& path);

  std::string const& root();
  std::string const& work();
  static const char sep = '/';
}

std::string operator / (std::string const& lhs, std::string const& rhs);
