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
#include <puttle_proxy.h>

#include <string>

PuttleServer::PuttleServer(const ios_deque& io_services, int port)
    : io_services_(io_services),
      acceptor_(*io_services.front(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    boost::asio::socket_base::reuse_address reuse_address(true);
    acceptor_.set_option(reuse_address);
    start_accept();
}

void PuttleServer::set_proxy_port(std::string port) {
    proxy_port_ = port;
}

void PuttleServer::set_proxy_host(std::string host) {
    proxy_host_ = host;
}

void PuttleServer::start_accept() {
    io_services_.push_back(io_services_.front());
    io_services_.pop_front();
    PuttleProxy::pointer new_proxy = PuttleProxy::create(*io_services_.front());

    acceptor_.async_accept(new_proxy->socket(),
                           boost::bind(&PuttleServer::handle_accept, this, new_proxy,
                                       boost::asio::placeholders::error));
}

void PuttleServer::handle_accept(PuttleProxy::pointer new_proxy, const boost::system::error_code& error) {
    if (!error) {
        new_proxy->forward_to(proxy_host_, proxy_port_);
        start_accept();
    }
}
