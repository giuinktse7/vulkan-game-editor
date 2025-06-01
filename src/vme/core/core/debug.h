#pragma once

#include <cassert>
#include <exception>

#include "logger.h"

#include "definitions.h"

inline bool DEBUG_FLAG_ACTIVE = false;

class GeneralDebugException : public std::exception
{
  public:
    GeneralDebugException(const char *message)
        : message(message) {}
    virtual const char *what() const throw()
    {
        return message;
    }

    const char *message;
};

#define ABORT_PROGRAM(message)                                                                       \
    do                                                                                               \
    {                                                                                                \
        std::ostringstream s;                                                                        \
        s << "[ERROR] " << __FILE__ << ", line " << (unsigned)(__LINE__) << ": " << message << "\n"; \
        VME_LOG_D(s.str());                                                                          \
        throw GeneralDebugException(s.str().c_str());                                                \
    }                                                                                                \
    while (false)

#define NOT_IMPLEMENTED_ABORT() ABORT_PROGRAM("Not implemented yet.");

#ifdef _DEBUG_VME
#define DEBUG_ASSERT(exp, msg)  \
    do                          \
        if (!(exp))             \
        {                       \
            ABORT_PROGRAM(msg); \
        }                       \
        else                    \
        {                       \
        }                       \
    while (false)
#else
#define DEBUG_ASSERT(exp, msg) \
    do                         \
    {                          \
    }                          \
    while (0)
#endif

#define FILE_AND_LINE_STR (__FILE__ + std::string("(" + std::to_string(__LINE__) + "): "))
