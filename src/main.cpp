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

#include <iostream>
#include <string>
#include <deque>

#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

int main(int argc, char** argv) {
    try {
        int thread_num = 2;
        int port = 8888;

        if (argc > 1)
            thread_num = boost::lexical_cast<int>(argv[1]);
        if (argc > 2)
            port = boost::lexical_cast<int>(argv[2]);

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
        puttle_server.set_proxy_host("oe");
        puttle_server.set_proxy_port("3128");

        thr_grp.join_all();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }


    return 0;
}
