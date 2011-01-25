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
#include <logger.h>

#include <log4cpp/BasicLayout.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/NDC.hh>

#include <string>

#include <boost/format.hpp>

namespace puttle {

void Logger::init() {
    // Initialiase the Logger object.
    Logger::instance();
}

void Logger::push_context(const std::string& context) {
    log4cpp::NDC::push(context);
}

void Logger::pop_context() {
    log4cpp::NDC::pop();
}

Logger::Log Logger::get_logger(const std::string& name) {
    return log4cpp::Category::getInstance(name);
}

void Logger::set_level(const std::string& level) {
    Logger::instance().set_level_impl(level);
}

Logger::Logger() :
    syslog(NULL),
    stdout(NULL) {
    syslog = new log4cpp::SyslogAppender("syslog", "puttle");
    syslog->setLayout(new log4cpp::BasicLayout());

    stdout = new log4cpp::OstreamAppender("puttle", &std::cout);
    stdout->setLayout(new log4cpp::BasicLayout());

    log4cpp::Category& root = log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::ERROR);
    root.addAppender(syslog);
    root.addAppender(stdout);
}

void Logger::set_level_impl(const std::string& level) {
    log4cpp::Category& root = log4cpp::Category::getRoot();
    log4cpp::Priority::Value v = log4cpp::Priority::ERROR;

    try {
        v = log4cpp::Priority::getPriorityValue(level);
    } catch(const std::invalid_argument& e) {
        root.errorStream() << level << " is not a valid debug level.";
        root.errorStream() << "Using 'ERROR' as the error level";
    }

    root.setPriority(v);
}
}

