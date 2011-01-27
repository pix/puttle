/*
 * Copyright (C) 2011 Camille Moncelier
 *
 * This file is part of puttle.
 *
 * puttle is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * puttle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with puttle in the COPYING.txt file.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef PUTTLE_SRC_LOGGER_H
#define PUTTLE_SRC_LOGGER_H

#include <puttle-common.h>
#include <singleton.h>

#include <string>
#include "log4cpp/Appender.hh"
#include "log4cpp/SyslogAppender.hh"
#include "log4cpp/Category.hh"


namespace puttle {

class Logger : public Singleton<Logger> {
    friend class Singleton<Logger>;
public:
    typedef log4cpp::Category& Log;
    typedef log4cpp::CategoryStream Stream;

    enum Priority {
        EMERG = log4cpp::Priority::EMERG,
        FATAL = log4cpp::Priority::FATAL,
        ALERT = log4cpp::Priority::ALERT,
        CRIT  = log4cpp::Priority::CRIT,
        ERROR = log4cpp::Priority::ERROR,
        WARN  = log4cpp::Priority::WARN,
        NOTICE  = log4cpp::Priority::NOTICE,
        INFO    = log4cpp::Priority::INFO,
        DEBUG   = log4cpp::Priority::DEBUG,
        NOTSET  = log4cpp::Priority::NOTSET,
    };

    static void init();
    static void set_level(const std::string& level);
    static Log get_logger(const std::string& name);
    static void push_context(const std::string& context);
    static void pop_context();

private:
    Logger();
    void set_level_impl(const std::string& level);

    log4cpp::SyslogAppender* syslog;
    log4cpp::Appender* stdout;
};
}

#endif /* end of include guard: PUTTLE_SRC_LOGGER_H */


