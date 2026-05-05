#pragma once

#include "F4SE/F4SE.h"

#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

inline void SetupLog()
{
	auto logsFolder = F4SE::log::log_directory();
	if (!logsFolder) {
		F4SE::stl::report_and_fail("F4SE log_directory not provided.");
	}

	auto logFilePath = *logsFolder / "RainSplashesF4SE.log";
	auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
	auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
	spdlog::set_default_logger(std::move(loggerPtr));
	spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
#ifndef NDEBUG
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);
#else
	spdlog::set_level(spdlog::level::info);
	spdlog::flush_on(spdlog::level::info);
#endif
}
