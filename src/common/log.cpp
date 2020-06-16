#include "sapien_vulkan/common/log.h"
#include <spdlog/sinks/stdout_color_sinks.h>


namespace svulkan {
namespace log {

std::shared_ptr<spdlog::logger> getLogger() {
  static auto logger = spdlog::stderr_color_mt("svulkan");
  return logger;
}

}
}
