#pragma once

#include "Core.hpp"
#include <spdlog/spdlog.h>

class Log
{
public:
	static void init();
	inline static std::shared_ptr<spdlog::logger>& getLogger() { return mLogger; }

private:
	static std::shared_ptr<spdlog::logger> mLogger;
};

// Log macros
#define APP_TRACE(...)	::Log::getLogger()->trace(__VA_ARGS__)
#define APP_INFO(...)	::Log::getLogger()->info(__VA_ARGS__)
#define APP_WARN(...)	::Log::getLogger()->warn(__VA_ARGS__)
#define APP_ERROR(...)	::Log::getLogger()->error(__VA_ARGS__)
#define APP_FATAL(...)	::Log::getLogger()->fatal(__VA_ARGS__)
