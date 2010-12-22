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
#ifndef PUTTLE_SRC_PUTTLE_AUTHENTICATOR_H
#define PUTTLE_SRC_PUTTLE_AUTHENTICATOR_H

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

namespace puttle {

class Authenticator;
class Authenticator {
public:
    typedef enum {
        AUTH_NONE,
        AUTH_BASIC,
        AUTH_INVALID,
    } Method;

    typedef boost::shared_ptr<Authenticator> pointer;
    static pointer create(Authenticator::Method m, const std::string& user, const std::string& pass);
    static Method get_method(const std::string& method);

    Authenticator(const std::string& user, const std::string& pass);
    virtual ~Authenticator();

    virtual bool has_token() = 0;
    virtual std::string get_token() = 0;

protected:
    std::string user_;
    std::string pass_;

private:
    static std::map<std::string, Method> method_names;
};

class BasicAuthenticator : public Authenticator {
public:
    BasicAuthenticator(const std::string& user, const std::string& pass);
    virtual bool has_token();
    virtual std::string get_token();
};

class NoneAuthenticator : public Authenticator {
public:
    NoneAuthenticator();
    virtual bool has_token();
    virtual std::string get_token();
};
}

#endif /* end of include guard: PUTTLE_SRC_PUTTLE_AUTHENTICATOR_H */
