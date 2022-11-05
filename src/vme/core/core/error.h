#pragma once

#include <stdexcept>
#include <string>

class MapLoadError : public std::runtime_error
{
  public:
    MapLoadError(const std::string &message)
        : std::runtime_error(message) {}
};