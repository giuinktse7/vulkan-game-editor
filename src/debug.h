#pragma once

#include <string>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>

#include "logger.h"

#if defined(_DEBUG) || !defined(QT_NO_DEBUG)
#define __DEBUG__VME
#endif

class GeneralDebugException : public std::exception
{
public:
  GeneralDebugException(const char *message) : message(message)
  {
  }
  virtual const char *what() const throw()
  {
    return message;
  }

  const char *message;
};

namespace debug
{
#define ABORT_PROGRAM(message)                                                                          \
  do                                                                                                    \
  {                                                                                                     \
    std::ostringstream s;                                                                               \
    s << "[ERROR] " << __FILE__ << ", line " << (unsigned)(__LINE__) << ": " << (message) << std::endl; \
    VME_LOG_D(s.str());                                                                                 \
    throw GeneralDebugException(s.str().c_str());                                                       \
  } while (false)

#ifdef __DEBUG__VME
#define DEBUG_ASSERT(exp, msg) \
  do                           \
    if (!(exp))                \
    {                          \
      ABORT_PROGRAM(msg);      \
    }                          \
    else                       \
    {                          \
    }                          \
  while (false)
#else
#define DEBUG_ASSERT(exp, msg) \
  do                           \
  {                            \
  } while (0)
#endif
} // namespace debug