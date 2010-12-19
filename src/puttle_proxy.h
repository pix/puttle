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
#ifndef PUTTLE_SRC_PUTTLE_PROXY_H
#define PUTTLE_SRC_PUTTLE_PROXY_H

#include <puttle-common.h>

#include <string>

using boost::asio::ip::tcp;

class PuttleProxy;
class PuttleProxy : public boost::enable_shared_from_this<PuttleProxy> {
public:

    enum _CONSTANTS {
        BUFFER_SIZE = 8192
    };

    typedef boost::shared_ptr<PuttleProxy> pointer;

    static pointer create(boost::asio::io_service& io_service) {
        return pointer(new PuttleProxy(io_service));
    }

    explicit PuttleProxy(boost::asio::io_service& io_service);
    ~PuttleProxy();

    tcp::socket& socket();
    void start_forwarding();
    void forward_to(std::string host, std::string port);

private:
    void setup_proxy();

    void handle_proxy_connect(const boost::system::error_code& error);
    void handle_proxy_response(const boost::system::error_code& error,
                               size_t bytes_transferred);

    void handle_resolve(const boost::system::error_code& error,
                        tcp::resolver::iterator endpoint_iterator);
    void handle_connect(const boost::system::error_code& error,
                        tcp::resolver::iterator endpoint_iterator);


    void handle_server_read(const boost::system::error_code& error,
                            size_t bytes_transferred);
    void handle_client_read(const boost::system::error_code& error,
                            size_t bytes_transferred);
    void handle_client_write(const boost::system::error_code& error);
    void handle_server_write(const boost::system::error_code& error);

    void shutdown();
    tcp::socket client_socket_;
    tcp::socket server_socket_;
    tcp::resolver resolver_;

    boost::array<char, BUFFER_SIZE> client_data_;
    boost::array<char, BUFFER_SIZE> server_data_;
    std::string server_headers_;
};


#endif /* end of include guard: PUTTLE_SRC_PUTTLE_PROXY_H */
