#pragma once

#include <iostream>
#include <string_view>
#include <string>

#define LOG_INFO std::cout << info

namespace Logger
{
  std::ostream &info();
  void info(std::string &s);

  std::ostream &debug();
  void debug(const char *s);
  void debug(std::string s);

  std::ostream &error();
  void error(std::string &s);
  void error(std::string s);
} // namespace Logger
