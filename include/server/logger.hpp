#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "logger/log.hpp"

extern logging::logger<logging::file_log_policy> log_inst;

#ifdef LOGGING_LEVEL_1

#define LOG log_inst.print<logging::severity_type::Debug>
#define LOG_ERR log_inst.print<logging::severity_type::Error>
#define LOG_WARN log_inst.print<logging::severity_type::Warning>

#else 

#define LOG(...)
#define LOG_ERROR(...)
#define LOG_WARN(...)

#endif

#ifdef LOGGING_LEVEL_2

#define ELOG log_inst.print<logging::severity_type::Debug>
#define ELOG_ERR log_inst.print<logging::serenity_type::Error>
#define ELOG_WARN log_ins.print<logging::serenity_type::Warning>

#else 

#define ELOG(...)
#define ELOG_ERROR(...)
#define ELOG_WARN(...)

#endif

#endif // LOGGER_HPP