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

#include <cstdint>
#include <config.h>

#include <map>
#include <string>

#include <boost/random.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/once.hpp>

namespace puttle {

class Authenticator;
class Authenticator {
public:
    typedef boost::mt19937 nonce_rng;

    typedef enum {
        AUTH_NONE,
        AUTH_BASIC,
        AUTH_DIGEST,
        AUTH_INVALID,
    } Method;

    typedef std::map<std::string, std::string> headers_map;

    typedef boost::shared_ptr<Authenticator> pointer;
    static pointer create(Authenticator::Method m, const std::string& user, const std::string& pass, const std::string& host, const std::string& port);
    static Method get_method(const std::string& method);

    Authenticator(const std::string& user, const std::string& pass);
    virtual ~Authenticator();

    virtual bool has_token() = 0;
    virtual std::string get_token() = 0;
    virtual bool has_error() = 0;
    virtual void set_headers(const headers_map& headers);

#ifdef HAVE_DEBUG
    // Used in unit tests
    inline static void reset_rng() {
        rng.seed(0);
    }
#endif

protected:
    static nonce_rng rng;
    static boost::once_flag init_rng_;
    static void init_rng();

    std::string user_;
    std::string pass_;
    headers_map headers_;

private:
    static std::map<std::string, Method> method_names;
};

class BasicAuthenticator : public Authenticator {
public:
    BasicAuthenticator(const std::string& user, const std::string& pass);
    virtual bool has_token();
    virtual std::string get_token();
    virtual bool has_error();

private:
    int retries;
};

class DigestAuthenticator : public Authenticator {
public:

    DigestAuthenticator(const std::string& user, const std::string& pass, const std::string& host, const std::string& port);
    virtual bool has_token();
    virtual std::string get_token();
    virtual bool has_error();
    virtual void set_headers(const headers_map& headers);
private:
    std::string get_md5(const std::string& str);
    std::string get_cnonce(size_t len);
    std::string get_response();
    std::string to_hex(uint8_t* data, size_t len);
    std::string find_quoted(const std::string& name, const std::string& header);

    int retries;
    int nonce_count_;
    std::string nc_;
    std::string host_;
    std::string port_;
    std::string realm_;
    std::string qop_;
    std::string nonce_;
    std::string cnonce_;
    std::string opaque_;
    std::string response_;
};

class NoneAuthenticator : public Authenticator {
public:
    NoneAuthenticator();
    virtual bool has_token();
    virtual std::string get_token();
    virtual bool has_error();

private:
    int retries;
};
}

#endif /* end of include guard: PUTTLE_SRC_PUTTLE_AUTHENTICATOR_H */
