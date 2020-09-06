
#include "util.h"
#include "graphics/validation.h"

#include <algorithm>

util::Size::Size(int width, int height) : w(width), h(height) {}

std::vector<const char *> getRequiredExtensions()
{
  // uint32_t glfwExtensionCount = 0;

  // const char **glfwExtensions;
  // glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  // std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  // if (Validation::enableValidationLayers)
  // {
  //   extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  // }
  std::vector<const char *> v;

  return v;
}

void to_lower_str(std::string &source)
{
  std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void to_upper_str(std::string &source)
{
  std::transform(source.begin(), source.end(), source.begin(), toupper);
}

std::string as_lower_str(std::string s)
{
  to_lower_str(s);
  return s;
}
