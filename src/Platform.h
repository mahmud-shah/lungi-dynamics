#pragma once

#include <string>

// Operating-system specific features live behind this small interface, so a
// later web or mobile port only has to replace PlatformWin.cpp.
namespace Platform {

bool chooseImageFile(std::string& path);

} // namespace Platform
