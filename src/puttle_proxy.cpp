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
#include <puttle_proxy.h>
#include <linux/netfilter_ipv4.h>

#include <map>
#include <string>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace puttle {

typedef boost::asio::detail::socket_option::integer<IPPROTO_IP, IP_TTL> time_to_live;

PuttleProxy::PuttleProxy(boost::asio::io_service& io_service )  // NOLINT
    : client_socket_(io_service),
      server_socket_(io_service),
      resolver_(io_service) {
}

PuttleProxy::~PuttleProxy() {
    client_socket_.close();
    server_socket_.close();
}

tcp::socket& PuttleProxy::socket() {
    return client_socket_;
}

void PuttleProxy::start_forwarding() {
    boost::system::error_code ec;
    handle_server_write(ec);
    handle_client_write(ec);
}

void PuttleProxy::set_proxy_user(const std::string& user) {
    proxy_user_ = user;
}

void PuttleProxy::set_proxy_pass(const std::string& pass) {
    proxy_pass_ = pass;
}


void PuttleProxy::forward_to(std::string host, std::string port) {
    host_ = host;
    port_ = port;
    resolve_destination();
}

void PuttleProxy::resolve_destination() {
    tcp::resolver::query query(host_, port_);
    resolver_.async_resolve(query,
                            boost::bind(&PuttleProxy::handle_resolve, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::iterator));
}

void PuttleProxy::handle_resolve(const boost::system::error_code& error,
                                 tcp::resolver::iterator endpoint_iterator) {
    if (!error) {
        // Attempt a connection to the first endpoint in the list. Each endpoint
        // will be tried until we successfully establish a connection.
        tcp::endpoint endpoint = *endpoint_iterator;

        server_socket_.async_connect(endpoint,
                                     boost::bind(&PuttleProxy::handle_connect, shared_from_this(),
                                             boost::asio::placeholders::error, ++endpoint_iterator));
        time_to_live ttl(42);
        server_socket_.set_option(ttl);
    } else {
        shutdown();
    }
}

void PuttleProxy::handle_connect(const boost::system::error_code& error,
                                 tcp::resolver::iterator endpoint_iterator) {
    if (!error) {
        setup_proxy();
    } else if (endpoint_iterator != tcp::resolver::iterator()) {
        // The connection failed. Try the next endpoint in the list.
        server_socket_.close();
        tcp::endpoint endpoint = *endpoint_iterator;
        server_socket_.async_connect(endpoint,
                                     boost::bind(&PuttleProxy::handle_connect, shared_from_this(),
                                             boost::asio::placeholders::error, ++endpoint_iterator));
    } else {
        shutdown();
    }
}

void PuttleProxy::setup_proxy() {
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int32_t _ip;
    uint16_t _port;

    getsockopt(client_socket_.native(), SOL_IP, SO_ORIGINAL_DST, (struct sockaddr*) &client, &len);
    _ip = ntohl(client.sin_addr.s_addr);
    _port = ntohs(client.sin_port);

    boost::format fmt = boost::format("CONNECT %1%:%2% HTTP/1.1\r\nHost: %1%:%2%\r\n") % (boost::asio::ip::address_v4(_ip)) % _port;
    std::string connect_string = fmt.str();

    if (authenticator_ != NULL && authenticator_->has_token())
        connect_string += authenticator_->get_token();

    connect_string += "\r\n";

    boost::asio::async_write(server_socket_,
                             boost::asio::buffer(connect_string),
                             boost::bind(&PuttleProxy::handle_proxy_connect, shared_from_this(),
                                         boost::asio::placeholders::error));
}

void PuttleProxy::handle_proxy_connect(const boost::system::error_code& error) {
    boost::asio::async_read(
        server_socket_,
        boost::asio::buffer(server_data_, BUFFER_SIZE),
        boost::asio::transfer_at_least(1),
        boost::bind(&PuttleProxy::handle_proxy_response, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void PuttleProxy::handle_proxy_response(const boost::system::error_code& error,
                                        size_t bytes_transferred) {
    if (!error) {
        if (server_headers_.empty())
            server_headers_ = std::string(server_data_.data(), bytes_transferred);
        else
            server_headers_ += std::string(server_data_.data(), bytes_transferred);

        if (server_headers_.find("\r\n\r\n") == std::string::npos &&
                server_headers_.find("\n\n") == std::string::npos) {
            // Need more headers
            boost::asio::async_read(
                server_socket_,
                boost::asio::buffer(server_data_, BUFFER_SIZE),
                boost::asio::transfer_at_least(1),
                boost::bind(&PuttleProxy::handle_proxy_response, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        } else {
            check_proxy_response();
        }
    } else {
        shutdown();
    }
}

void PuttleProxy::check_proxy_response() {
    size_t pos = server_headers_.find("HTTP/");

    // HTTP/ Should be the first header line
    if ( pos != 0 )
        shutdown_error();

    size_t first_space = server_headers_.find_first_of(" ", pos);
    std::string status = server_headers_.substr(first_space + 1, 3);

    std::string headers(server_headers_);
    std::string::size_type index;
    std::string line;
    while ((index = headers.find("\r\n")) != std::string::npos) {
        line = headers.substr(0, index);
        headers.erase(0, index+2);
        if (line == "")
            break;
        index = line.find(": ");
        if (index == std::string::npos) {
            continue;  // Probably the status line
        }
        headers_.insert(std::make_pair(line.substr(0, index), line.substr(index+2)));
    }

    int http_status = boost::lexical_cast<int>(status);
    switch (http_status) {
    case 200:
        start_forwarding();
        break;
    case 407:
        handle_proxy_auth();
        break;
    default:
        shutdown();
        break;
    }
}

void PuttleProxy::handle_proxy_auth() {
    if (headers_.find("Proxy-Authenticate") != headers_.end()) {
        if ( authenticator_ == NULL ) {
            std::string method = headers_["Proxy-Authenticate"];
            method = method.substr(0, method.find_first_of(" "));

            Authenticator::Method m = Authenticator::get_method(method);
            authenticator_ = Authenticator::create(m, proxy_user_, proxy_pass_);
        }

        server_headers_.clear();
        headers_.clear();
        server_socket_.close();

        resolve_destination();
    } else {
        /* FIXME: Can we get here ? */
        shutdown_error();
    }
}

void PuttleProxy::handle_server_read(const boost::system::error_code& error,
                                     size_t bytes_transferred) {
    if (!error) {
        boost::asio::async_write(client_socket_,
                                 boost::asio::buffer(server_data_, bytes_transferred),
                                 boost::bind(&PuttleProxy::handle_client_write, shared_from_this(),
                                             boost::asio::placeholders::error));

        boost::system::error_code ec;
        handle_server_write(ec);
    } else {
        shutdown();
    }
}

void PuttleProxy::handle_client_read(const boost::system::error_code& error,
                                     size_t bytes_transferred) {
    if (!error) {
        boost::asio::async_write(server_socket_,
                                 boost::asio::buffer(client_data_, bytes_transferred),
                                 boost::bind(&PuttleProxy::handle_server_write, shared_from_this(),
                                             boost::asio::placeholders::error));

        boost::system::error_code ec;
        handle_client_write(ec);
    } else {
        shutdown();
    }
}

void PuttleProxy::handle_client_write(const boost::system::error_code& error) {
    if (!error) {
        boost::asio::async_read(
            client_socket_,
            boost::asio::buffer(client_data_, BUFFER_SIZE),
            boost::asio::transfer_at_least(1),
            boost::bind(&PuttleProxy::handle_client_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    } else {
        shutdown();
    }
}

void PuttleProxy::handle_server_write(const boost::system::error_code& error) {
    if (!error) {
        boost::asio::async_read(
            server_socket_,
            boost::asio::buffer(server_data_, BUFFER_SIZE),
            boost::asio::transfer_at_least(1),
            boost::bind(&PuttleProxy::handle_server_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    } else {
        shutdown();
    }
}

void PuttleProxy::shutdown_error() {
    /* TODO: Provide an error message */
    shutdown();
}

void PuttleProxy::shutdown() {
    client_socket_.close();
    server_socket_.close();
}
}
