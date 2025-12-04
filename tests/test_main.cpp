#include "../utils/flx_env.h"
#include <filesystem>

static void load_test_environment() {
  std::filesystem::path test_dir = std::filesystem::path(__FILE__).parent_path();
  std::filesystem::path project_root = test_dir.parent_path();
  std::filesystem::path env_file = project_root / ".env";

  if (std::filesystem::exists(env_file)) {
    load_env_file(flx_string(env_file.string().c_str()));
  }
}

struct EnvironmentLoader {
  EnvironmentLoader() { load_test_environment(); }
};
static EnvironmentLoader env_loader;

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// Main test runner - automatically includes all test_*.cpp files
// via CMake auto-discovery