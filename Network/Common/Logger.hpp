#pragma once

#include <WinSock2.h>

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

#define LOG_DEBUG(fmt, ...) \
	NetUtils::LogInfo(NetUtils::LogLevel::kDebug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
	NetUtils::LogInfo(NetUtils::LogLevel::kInfo, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
	NetUtils::LogInfo(NetUtils::LogLevel::kWarn, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                             \
	NetUtils::LogError(NetUtils::LogLevel::kError, fmt, \
					   std::source_location::current(), ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)                             \
	NetUtils::LogError(NetUtils::LogLevel::kFatal, fmt, \
					   std::source_location::current(), ##__VA_ARGS__)

namespace NetUtils {
enum class LogLevel : std::uint8_t { kDebug, kInfo, kWarn, kError, kFatal };
constexpr inline LogLevel kLogLevel = LogLevel::kDebug;
constexpr std::array<std::string_view, 5> kLevelStrings{
	"\033[36mDEBUG\033[0m",	  // Cyan
	"\033[32mINFO \033[0m",	  // Green
	"\033[33mWARN \033[0m",	  // Yellow
	"\033[31mERROR\033[0m",	  // Red
	"\033[1;31mFATAL\033[0m"  // Bold Red
};

constexpr std::string_view GetLevelStr(LogLevel level) {
	return kLevelStrings[static_cast<size_t>(level)];
}

template <typename... Args>
void LogInfo(LogLevel level, std::string_view fmt_str, Args&&... args) {
	auto now = std::chrono::system_clock::now();
	try {
		if (level < kLogLevel) return;
		std::string msg = std::vformat(fmt_str, std::make_format_args(args...));

		std::cout << std::format("[{:%F %T}][{}]{}\n", now, GetLevelStr(level),
								 msg);
	} catch (const std::format_error& e) {
		std::cerr << std::format("[{:%F %T}][{}]Log formatting error: {}\n",
								 now, GetLevelStr(level), e.what());
	}
}

template <typename... Args>
void LogError(LogLevel level, std::string_view fmt_str,
			  std::source_location location, Args&&... args) {
	auto now = std::chrono::system_clock::now();
	try {
		if (level < kLogLevel) return;
		std::string msg = std::vformat(fmt_str, std::make_format_args(args...));

		std::cerr << std::format("[{:%F %T}][{}]{}\n{}:{}\n", now,
								 GetLevelStr(level), msg, location.file_name(),
								 location.line());
		if (level == LogLevel::kFatal) {
			std::cout.flush();
			std::cerr.flush();
			throw std::runtime_error(msg);
		}
	} catch (const std::format_error& e) {
		std::cerr << std::format(
			"[{:%F %T}][{}]Log formatting error: {}\n{}:{}\n", now,
			GetLevelStr(level), e.what(), location.file_name(),
			location.line());
	}
}
}  // namespace NetUtils
