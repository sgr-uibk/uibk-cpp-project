#include "Utilities.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> createConsoleLogger(std::string const &name)
{
	auto logger = spdlog::stdout_color_mt(name);
	logger->set_pattern(DEFAULT_PATTERN);
	return logger;
}

// https://github.com/gabime/spdlog/wiki/Creating-loggers#creating-loggers-with-multiple-sinks
std::shared_ptr<spdlog::logger> createConsoleAndFileLogger(
	std::string const &name,
	spdlog::level::level_enum const logLevel)
{
	std::string const logFile = name + ".log";
	std::vector<spdlog::sink_ptr> sinks{
		std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
		std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile),
	};
	auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
	spdlog::register_logger(logger); // for spdlog::get()
	logger->set_level(logLevel);
	logger->set_pattern(DEFAULT_PATTERN);

	return logger;
}
