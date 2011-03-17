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
#include <logger.h>
#include <linux/netfilter_ipv4.h>

#include <map>
#include <string>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace puttle {

typedef boost::asio::detail::socket_option::integer<IPPROTO_IP, IP_TTL> time_to_live;

PuttleProxy::PuttleProxy(boost::asio::io_service& io_service, proxy_vector proxies)  // NOLINT
    : client_socket_(io_service),
      server_socket_(io_service),
      resolver_(io_service),
      proxies_(proxies),
      log(Logger::get_logger("puttle.puttle-proxy")) {

    std::random_shuffle(proxies_.begin(), proxies_.end() );
}

PuttleProxy::~PuttleProxy() {
    client_socket_.close();
    server_socket_.close();
}

tcp::socket& PuttleProxy::socket() {
    return client_socket_;
}

void PuttleProxy::start_forwarding() {
    log.infoStream() <<
                     client_socket_.remote_endpoint().address().to_string() << ":" << client_socket_.remote_endpoint().port()
                     << " -> " << dest_host_ << ":" << dest_port_ << " via: "
                     << (*it_proxy)->host << ":" << (*it_proxy)->port;

    boost::system::error_code ec;
    handle_server_write(ec);
    handle_client_write(ec);
}

void PuttleProxy::init_forward() {
    it_proxy = proxies_.begin();
    resolve_destination();
}

void PuttleProxy::resolve_destination() {
    if (it_proxy != proxies_.end()) {
        Proxy& p = *(it_proxy->get());

        log.debug("Resolving %s:%u", p.host.c_str(), p.port);
        tcp::resolver::query query(p.host, boost::lexical_cast<std::string>(p.port));
        resolver_.async_resolve(query,
                                boost::bind(&PuttleProxy::handle_resolve, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::iterator));
    } else {
        shutdown();
    }
}

void PuttleProxy::handle_resolve(const boost::system::error_code& error,
                                 tcp::resolver::iterator endpoint_iterator) {
    if (!error) {
        // Attempt a connection to the first endpoint in the list. Each endpoint
        // will be tried until we successfully establish a connection.
        tcp::endpoint endpoint = *endpoint_iterator;

        try {
            // If the socket was previously opened, close it.
            if (server_socket_.is_open())
                server_socket_.close();

            // Opens the socket so we can set the ttl option
            server_socket_.open(boost::asio::ip::tcp::v4());

            time_to_live ttl(42);
            server_socket_.set_option(ttl);

            boost::asio::socket_base::keep_alive keep_alive(true);
            server_socket_.set_option(keep_alive);

            server_socket_.async_connect(endpoint,
                                         boost::bind(&PuttleProxy::handle_connect, shared_from_this(),
                                                 boost::asio::placeholders::error, ++endpoint_iterator));
        } catch(const boost::system::system_error &e) {
            log.errorStream() << "Could not open a socket: " << e.what();
            shutdown_error();
        }
    } else {
        log.error("Unable to resolve: %s:%u, skipping", (*it_proxy)->host.c_str(), (*it_proxy)->port);
        it_proxy++;
        resolve_destination();
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
        log.error("Unable to connect to %s:%u, skippin", (*it_proxy)->host.c_str(), (*it_proxy)->port);
        it_proxy++;
        resolve_destination();
    }
}

void PuttleProxy::setup_proxy() {
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    uint32_t _ip;
    uint16_t _port;

    getsockopt(client_socket_.native(), SOL_IP, SO_ORIGINAL_DST, (struct sockaddr*) &client, &len);
    _ip = ntohl(client.sin_addr.s_addr);
    _port = ntohs(client.sin_port);

    dest_host_ = (boost::format("%s") %boost::asio::ip::address_v4(_ip)).str();
    dest_port_ = boost::lexical_cast<std::string>(_port);

    boost::format fmt = boost::format("CONNECT %1%:%2% HTTP/1.1\r\n" \
                                      "User-Agent: Mozilla/5.0 (X11; U; AmigaOS x86_64; eo-EO; rv:42.6.6)r\n" \
                                      "Proxy-Connection: keep-alive\r\n" \
                                      "Host: %1%:%2%\r\n") % dest_host_ % dest_port_;
    std::string connect_string = fmt.str();

    if (authenticator_ != NULL) {
        if (authenticator_->has_error()) {
            log.error("Unable to authenticate the request to: %s:%s", dest_host_.c_str(), dest_port_.c_str());
            log_headers(Logger::ERROR, "Headers", authenticator_->get_headers());
            log.errorStream() << "Answer:";
            log.errorStream() << authenticator_->get_token();
            shutdown();
        } else if (authenticator_->has_token()) {
            std::string token = authenticator_->get_token();
            connect_string += token;
            log.debugStream() << "Autenticator: " << token;
        }
    }

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
        log.error("Error while reading proxy response: %s", error.message().c_str());
        shutdown();
    }
}

void PuttleProxy::check_proxy_response() {
    size_t pos = server_headers_.find("HTTP/");

    // HTTP/ Should be the first header line
    if ( pos != 0 ) {
        log.error("Unable to parse the proxy answer, first line contains garbage");
        log.debug(server_headers_);
        shutdown_error();
    }

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

    int http_status = 0;
    try {
        http_status = boost::lexical_cast<int>(status);
    } catch(const boost::bad_lexical_cast& e) {
        // We failed to parse the proxy answer
        log.error("Unable to parse the proxy answer");
        shutdown_error();
    }

    log.debugStream() << "Got a \"" << status << "\" status code";

    switch (http_status) {
    case 200:
        start_forwarding();
        break;
    case 407:
        handle_proxy_auth();
        break;
    default:
        log.error("Unknown http status code: %d", http_status);
        log_headers(Logger::ERROR, "Proxy error", headers_);
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
            authenticator_ = Authenticator::create(m, *(it_proxy->get()), dest_host_, dest_port_);
        }

        if (authenticator_ != NULL) {
            authenticator_->set_headers(headers_);
        }

        log_headers(Logger::DEBUG, "Authentication", headers_);

        server_headers_.clear();
        headers_.clear();
        server_socket_.close();

        resolve_destination();
    } else {
        /* FIXME: Can we get here ? */
        shutdown_error();
    }
}

void PuttleProxy::log_headers(const Logger::Priority& priority, std::string context, const headers_map& headers) {
    if (!log.isPriorityEnabled(priority))
            return;

    Logger::push_context(context);
    headers_map::const_iterator end = headers.end();
    for (headers_map::const_iterator it = headers.begin(); it != end; ++it) {
        log.getStream(priority) << it->first << ": " << it->second;
    }
    Logger::pop_context();
}

void PuttleProxy::handle_server_read(const boost::system::error_code& error,
                                     size_t bytes_transferred) {
    if (!error) {
        boost::asio::async_write(client_socket_,
                                 boost::asio::buffer(server_data_, bytes_transferred),
                                 boost::bind(&PuttleProxy::handle_server_write, shared_from_this(),
                                             boost::asio::placeholders::error));

    } else {
        shutdown();
    }
}

void PuttleProxy::handle_client_read(const boost::system::error_code& error,
                                     size_t bytes_transferred) {
    if (!error) {
        boost::asio::async_write(server_socket_,
                                 boost::asio::buffer(client_data_, bytes_transferred),
                                 boost::bind(&PuttleProxy::handle_client_write, shared_from_this(),
                                             boost::asio::placeholders::error));

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
