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
#ifndef PUTTLE_SRC_PROXY_H
#define PUTTLE_SRC_PROXY_H

#include <puttle-common.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace puttle {

struct Proxy {
public:

    Proxy();
    Proxy(std::string host_, uint16_t port_ = 3128,
          std::string username_ = "", std::string password_ = "");

    static Proxy parse(std::string url);
    bool is_valid();

    uint16_t port;
    std::string host;
    std::string username;
    std::string password;

    static const Proxy invalid_proxy;
private:
    static std::string url_decode(const std::string& uri);
};

typedef std::vector<boost::shared_ptr<Proxy> > proxy_vector;
}

#endif /* end of include guard: PUTTLE_SRC_PROXY_H */
