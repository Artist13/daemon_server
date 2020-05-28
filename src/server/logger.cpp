#include "server/logger.hpp"

logging::logger<logging::file_log_policy> log_inst("/var/log/my_daemon.log");