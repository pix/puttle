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
#include <authenticator.h>

#include <boost/assign.hpp>
#include <boost/format.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <map>
#include <string>
#include <stdexcept>

namespace puttle {

typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<std::string::const_iterator, 6, 8> > base64_t;

Authenticator::Authenticator(const std::string& user, const std::string& pass) :
    user_(user),
    pass_(pass) {
}

Authenticator::~Authenticator() {
}

std::map<std::string, Authenticator::Method> Authenticator::method_names = boost::assign::map_list_of
        ("Invalid",  Authenticator::AUTH_INVALID)
        ("Basic",  Authenticator::AUTH_BASIC)
        ("None", Authenticator::AUTH_NONE);

Authenticator::Method Authenticator::get_method(const std::string& method) {
    if (method_names.count(method))
        return method_names[method];
    else
        return Authenticator::AUTH_INVALID;
}

Authenticator::pointer Authenticator::create(Authenticator::Method m, const std::string& user, const std::string& pass) {
    switch (m) {
    case Authenticator::AUTH_BASIC:
        return pointer(new BasicAuthenticator(user, pass));
    case Authenticator::AUTH_NONE:
        return pointer(new NoneAuthenticator());
    default:
        throw std::runtime_error(std::string("Invalid auth_method"));
    }
}

NoneAuthenticator::NoneAuthenticator() : Authenticator(std::string(), std::string()) {
}

bool NoneAuthenticator::has_token() {
    return false;
}

std::string NoneAuthenticator::get_token() {
    return std::string();
}

BasicAuthenticator::BasicAuthenticator(const std::string& user, const std::string& pass) : Authenticator(user, pass) {
}

bool BasicAuthenticator::has_token() {
    return true;
}

std::string BasicAuthenticator::get_token() {
    boost::format cred_format("%s:%s");
    cred_format % user_ % pass_;
    std::string cred = cred_format.str();

    std::string enc(base64_t(cred.begin()), base64_t(cred.end()));
    enc.insert(enc.end(), enc.size() % 4, '=');

    boost::format fmt("Proxy-Authorization: Basic %s\r\n");
    fmt % enc;

    return fmt.str();
}
}
