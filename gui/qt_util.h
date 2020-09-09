#pragma once

#include <QPixmap>

class MainApplication;

namespace QtUtil
{
  QPixmap itemPixmap(uint16_t serverId);
  MainApplication *qtApp();

} // namespace QtUtil