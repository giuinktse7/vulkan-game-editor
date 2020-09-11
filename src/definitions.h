#pragma once

// Version info
// xxyyzzt (major, minor, subversion)
#define __VME_VERSION_MAJOR__ 0
#define __VME_VERSION_MINOR__ 1
#define __VME_SUBVERSION__ 0

#define MAKE_VERSION_ID(major, minor, subversion) \
    ((major)*10000000 +                           \
     (minor)*100000 +                             \
     (subversion)*1000)

#define __VME_VERSION_ID__ MAKE_VERSION_ID( \
    __VME_VERSION_MAJOR__,                  \
    __VME_VERSION_MINOR__,                  \
    __VME_SUBVERSION__)

#ifdef __EXPERIMENTAL__
#define __VME_VERSION__ std::string(std::to_string(__VME_VERSION_MAJOR__) + "." + std::to_string(__VME_VERSION_MINOR__) + +" BETA")
#else
#define __VME_VERSION__ std::string(std::to_string(__VME_VERSION_MAJOR__) + "." + std::to_string(__VME_VERSION_MINOR__))
#endif

