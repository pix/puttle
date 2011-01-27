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
#include <puttle-common.h>
#include <puttle_server.h>
#include <logger.h>
#include <proxy.h>

#include <iostream>  // NOLINT
#include <fstream>   // NOLINT
#include <string>
#include <vector>
#include <deque>

#include <boost/any.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

using ::puttle::Proxy;
using ::puttle::Logger;
using ::puttle::PuttleServer;
using ::puttle::ios_deque;
using ::puttle::io_service_ptr;
using ::puttle::proxy_vector;

namespace po = ::boost::program_options;


namespace puttle {
/* Overload the program_options 'validate' function for
 * user-defined class.
 */
void validate(boost::any& v, const std::vector<std::string>& values, boost::shared_ptr<Proxy>*, int);
}

int main(int argc, char** argv) {
    Logger::init();
    try {
        int thread_num = 2;
        int port = 8888;

        proxy_vector proxies;
        std::string debug_level = "ERROR";
        std::string config_file;

        {
            po::options_description config_options("Configuration");
            config_options.add_options()
            ("num-threads,n", po::value<int>(&thread_num),
             "Number of threads")
            ("listen-port,l", po::value<int>(&port),
             "Port to listen to")
            ("config-file,c", po::value<std::string>(&config_file),
             "Configuration file");

            po::options_description auth_options("Remote proxy");
            auth_options.add_options()
            ("proxy,p", po::value<proxy_vector>(&proxies),
             "proxy value (in scheme://user:password@host:port form)\n" \
             "Only the http scheme is supported");

            po::options_description debug_options("Logging & Debugging");
            debug_options.add_options()
            ("verbosity,v", po::value<std::string>(&debug_level),
             "Sets the verbosity level: [EMERG | FATAL | ALERT | CRIT | ERROR | WARN | NOTICE | INFO | DEBUG]\n" \
             "If verbosity is > ERROR, then stdout logging is enabled\n" \
             "Be careful as it could leak your username and password");


            po::options_description all_opt;
            all_opt.add(config_options);
            all_opt.add(auth_options);
            all_opt.add(debug_options);

            all_opt.add_options()
            ("help,h", "print this message");

            po::variables_map vm;

            // Parse the command line
            try {
                po::store(po::command_line_parser(argc, argv)
                          .options(all_opt)
                          .run(),
                          vm);
                po::notify(vm);
            } catch(const po::error& e) {
                std::cerr << "Error in command line: " << e.what() << std::endl;
                return 1;
            }

            // If we need to read a config file
            if (vm.count("config-file")) {
                std::ifstream file(config_file.c_str());
                try {
                    po::store(parse_config_file(file, all_opt), vm);
                    po::notify(vm);
                } catch(const boost::program_options::error& e) {
                    std::cerr << "Error in config file: " << e.what() << std::endl;
                    return 1;
                }
            }


            if (vm.count("help") || !vm.count("proxy")) {
                std::cout << all_opt << std::endl;
                return 1;
            }
        }

        Logger::set_level(debug_level);
        Logger::Log log = Logger::get_logger("main");
        log.setPriority(Logger::INFO);

        log.info("Starting %s v%s", PACKAGE_NAME, PACKAGE_VERSION);
        log.info("Listening on port %d, forwarding to:", port);

        for (size_t i = 0; i < proxies.size(); i++) {
            log.info("        %s:%d", proxies[i]->host.c_str(), proxies[i]->port);
        }
        ios_deque io_services;
        std::deque<boost::asio::io_service::work> io_service_work;

        boost::thread_group thr_grp;

        for (int i  =  0; i < thread_num; ++i) {
            io_service_ptr ios(new boost::asio::io_service);
            io_services.push_back(ios);
            io_service_work.push_back(boost::asio::io_service::work(*ios));
            thr_grp.create_thread(boost::bind(&boost::asio::io_service::run, ios));
        }

        PuttleServer puttle_server(io_services, port, proxies);

        thr_grp.join_all();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }


    return 0;
}


/* Overload the program_options 'validate' function for
 * user-defined class.
 */
void puttle::validate(boost::any& v,                          // NOLINT: overloaded method signature
                      const std::vector<std::string>& values,
                      boost::shared_ptr<Proxy>*, int) {
    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);
    Proxy p = Proxy::parse(s);

    if (p.is_valid()) {
        v = boost::any(boost::shared_ptr<Proxy>(new Proxy(p)));
    } else {
#if BOOST_VERSION >= 104200
        throw po::validation_error(po::validation_error::invalid_option_value);
#else
        throw po::validation_error("Unable to parse proxy");
#endif
    }
}

