#include "../graphics/appearances.h"

template <typename T>
inline QDebug operator<<(QDebug d, const T &action)
{
  d << stringify(action).str();
  return d;
}
