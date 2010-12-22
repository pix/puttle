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
#include <puttle_server.h>
#include <config.h>

#include <iostream>  // NOLINT
#include <string>
#include <deque>

#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

using ::puttle::PuttleServer;
using ::puttle::ios_deque;
using ::puttle::io_service_ptr;

namespace po = ::boost::program_options;

int main(int argc, char** argv) {
    try {
        int thread_num = 2;
        int port = 8888;
        std::string proxy_host = "localhost";
        std::string proxy_port = "3128";

        std::string proxy_user;
        std::string proxy_pass;

        {
            po::options_description config_options("Configuration");
            config_options.add_options()
            ("num-threads,n", po::value<int>(&thread_num),
             "Number of threads")
            ("listen-port,l", po::value<int>(&port),
             "Port to listen to")
            ("proxy-port,P", po::value<std::string>(&proxy_port),
             "Destination proxy port")
            ("proxy-host,H", po::value<std::string>(&proxy_host),
             "Destination proxy host");

            po::options_description auth_options("Authentication");
            auth_options.add_options()
            ("user", po::value<std::string>(&proxy_user),
             "Proxy username")
            ("password", po::value<std::string>(&proxy_pass),
             "Proxy password")
            ("method", "Use 'method' for authentication");


            po::options_description all_opt;
            all_opt.add(config_options);
            all_opt.add(auth_options);

            all_opt.add_options()
            ("help,h", "print this message");

            po::variables_map vm;
            po::store(po::command_line_parser(argc, argv)
                  .options(all_opt)
                  .run(),
                  vm);
            po::notify(vm);

            if (vm.count("help")) {
                std::cout << all_opt << std::endl;
                return 1;
            }
        }

        std::cout << PACKAGE_NAME << " v" << PACKAGE_VERSION;
        std::cout << ". Listening on port: " << port << ", forwarding to: " << proxy_host << ":" << proxy_port << std::endl;

        ios_deque io_services;
        std::deque<boost::asio::io_service::work> io_service_work;

        boost::thread_group thr_grp;

        for (int i  =  0; i < thread_num; ++i) {
            io_service_ptr ios(new boost::asio::io_service);
            io_services.push_back(ios);
            io_service_work.push_back(boost::asio::io_service::work(*ios));
            thr_grp.create_thread(boost::bind(&boost::asio::io_service::run, ios));
        }
        PuttleServer puttle_server(io_services, port);
        puttle_server.set_proxy_host(proxy_host);
        puttle_server.set_proxy_port(proxy_port);
        puttle_server.set_proxy_user(proxy_user);
        puttle_server.set_proxy_pass(proxy_pass);

        thr_grp.join_all();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }


    return 0;
}
