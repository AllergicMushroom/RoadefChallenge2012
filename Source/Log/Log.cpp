#include "Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::mLogger;

void Log::init()
{
	spdlog::set_pattern("%^[%T] %n %l: %v%$");

	mLogger = spdlog::stdout_color_mt("Application");
	mLogger->set_level(spdlog::level::trace);

	APP_INFO("Initialised Loggers");
}
