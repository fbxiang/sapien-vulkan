#pragma once

#include <spdlog/spdlog.h>
#include <string_view>
#include <exception>

namespace svulkan {
namespace log {

std::shared_ptr<spdlog::logger> getLogger();

template<typename... Args>
inline void debug(spdlog::string_view_t fmt, const Args &...args) {
  getLogger()->debug(fmt, args...);
};

template<typename... Args>
inline void info(spdlog::string_view_t fmt, const Args &...args) {
  getLogger()->info(fmt, args...);
};

template<typename... Args>
inline void warn(spdlog::string_view_t fmt, const Args &...args) {
  getLogger()->warn(fmt, args...);
};

template<typename... Args>
inline void error(spdlog::string_view_t fmt, const Args &...args) {
  getLogger()->error(fmt, args...);
};

template<typename... Args>
inline void critical(spdlog::string_view_t fmt, const Args &...args) {
  getLogger()->critical(fmt, args...);
};

template<typename... Args>
inline void check(bool exp, spdlog::string_view_t fmt, const Args &...args) {
  if (!exp) {
    getLogger()->critical(fmt, args...);
    throw std::runtime_error("assertion failed");
  }
};

}
}
