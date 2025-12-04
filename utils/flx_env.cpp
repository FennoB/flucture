#include "flx_env.h"
#include <fstream>
#include <cstdlib>

void load_env_file(const flx_string& filepath) {
  std::ifstream file(filepath.c_str());
  if (!file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    flx_string flx_line(line.c_str());
    flx_line = flx_line.trim();

    if (flx_line.empty() || flx_line.starts_with("#")) {
      continue;
    }

    size_t pos = flx_line.find("=");
    if (pos == flx_string::npos) {
      continue;
    }

    flx_string key = flx_line.substr(0, pos).trim();
    flx_string value = flx_line.substr(pos + 1).trim();

    setenv(key.c_str(), value.c_str(), 1);
  }
}
