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
#ifndef PUTTLE_SRC_PUTTLE_SERVER_H
#define PUTTLE_SRC_PUTTLE_SERVER_H

#include <puttle-common.h>
#include <puttle_proxy.h>
#include <authenticator.h>

#include <string>
#include <deque>

namespace puttle {

using ::boost::asio::ip::tcp;

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef std::deque<io_service_ptr> ios_deque;

class PuttleServer {
public:

    PuttleServer(const ios_deque& io_services, int port);

    void start_accept();
    void set_proxy_port(const std::string& port);
    void set_proxy_host(const std::string& host);
    void set_proxy_user(const std::string& user);
    void set_proxy_pass(const std::string& pass);

private:
    void handle_accept(PuttleProxy::pointer new_proxy, const boost::system::error_code& error);
    std::string proxy_port_;
    std::string proxy_host_;
    std::string proxy_user_;
    std::string proxy_pass_;
    ios_deque io_services_;
    tcp::acceptor acceptor_;
};
}

#endif /* end of include guard: PUTTLE_SRC_PUTTLE_SERVER_H */


