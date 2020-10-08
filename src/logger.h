#pragma once

#include <iostream>
#include <string_view>
#include <string>
#include <sstream>

#ifdef QT_CORE_LIB
#include <QDebug>
#include <QString>
#endif

#include "definitions.h"

namespace Logger
{

#ifdef QT_CORE_LIB
  using MainLogger = QDebug;
  MainLogger info();
  MainLogger debug();

#else
  using MainLogger = std::ostream;
  MainLogger &info();
  MainLogger &debug();
#endif

  void info(std::string &s);
  void info(std::ostringstream &s);
  void debug(std::ostringstream &s);

  void debug(const char *s);
  void debug(std::string s);

  std::ostream &error();
  void error(std::string &s);
  void error(std::string s);

} // namespace Logger

#define VME_LOG(expr)         \
  do                          \
  {                           \
                              \
    std::ostringstream __s__; \
    __s__ << expr;            \
    Logger::info(__s__);      \
  } while (false)

#ifdef _DEBUG_VME
#define VME_LOG_D(expr)       \
  do                          \
  {                           \
                              \
    std::ostringstream __s__; \
    __s__ << expr;            \
    Logger::debug(__s__);     \
  } while (false)
#else
#define VME_LOG_D(expr) \
  do                    \
  {                     \
  } while (0)
#endif