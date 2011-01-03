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
#include <openssl/md5.h>

#include <boost/assign.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <map>
#include <string>
#include <stdexcept>

#define MD5_HASH_SIZE 16
#define HASH_HEX_SIZE (2 * MD5_HASH_SIZE)

namespace puttle {

typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<std::string::const_iterator, 6, 8> > base64_t;

Authenticator::Authenticator(const std::string& user, const std::string& pass) :
    user_(user),
    pass_(pass) {
    boost::call_once(Authenticator::init_rng, Authenticator::init_rng_);
}

Authenticator::~Authenticator() {
}

Authenticator::nonce_rng Authenticator::rng;
boost::once_flag Authenticator::init_rng_ = BOOST_ONCE_INIT;

void Authenticator::init_rng() {
    rng.seed(static_cast<unsigned int>(std::time(0)));
}

std::map<std::string, Authenticator::Method> Authenticator::method_names = boost::assign::map_list_of
        ("Invalid",  Authenticator::AUTH_INVALID)
        ("Digest",  Authenticator::AUTH_DIGEST)
        ("Basic",  Authenticator::AUTH_BASIC)
        ("None", Authenticator::AUTH_NONE);

Authenticator::Method Authenticator::get_method(const std::string& method) {
    if (method_names.count(method))
        return method_names[method];
    else
        return Authenticator::AUTH_INVALID;
}

Authenticator::pointer Authenticator::create(Authenticator::Method m, const std::string& user, const std::string& pass, const std::string& host, const std::string& port) {
    switch (m) {
    case Authenticator::AUTH_BASIC:
        return pointer(new BasicAuthenticator(user, pass));
    case Authenticator::AUTH_DIGEST:
        return pointer(new DigestAuthenticator(user, pass, host, port));
    case Authenticator::AUTH_NONE:
        return pointer(new NoneAuthenticator());
    default:
        return pointer();
    }
}

void Authenticator::set_headers(const headers_map& headers) {
    headers_ = headers;
}

NoneAuthenticator::NoneAuthenticator() : Authenticator(std::string(), std::string()) {
}

bool NoneAuthenticator::has_token() {
    return false;
}

bool NoneAuthenticator::has_error() {
    return false;
}

std::string NoneAuthenticator::get_token() {
    return std::string();
}


BasicAuthenticator::BasicAuthenticator(const std::string& user, const std::string& pass) : Authenticator(user, pass), retries(2) {
}

bool BasicAuthenticator::has_token() {
    return retries >= 0;
}

bool BasicAuthenticator::has_error() {
    return retries < 0;
}

std::string BasicAuthenticator::get_token() {
    --retries;

    boost::format cred_format("%s:%s");
    cred_format % user_ % pass_;
    std::string cred = cred_format.str();

    std::string enc(base64_t(cred.begin()), base64_t(cred.end()));
    enc.insert(enc.end(), enc.size() % 4, '=');

    boost::format fmt("Proxy-Authorization: Basic %s\r\n");
    fmt % enc;

    return fmt.str();
}

DigestAuthenticator::DigestAuthenticator(const std::string& user, const std::string& pass, const std::string& host, const std::string& port) :
    Authenticator(user, pass),
    retries(5),
    nonce_count_(0),
    host_(host),
    port_(port) {
}

bool DigestAuthenticator::has_token() {
    return retries >= 0;
}

bool DigestAuthenticator::has_error() {
    return retries < 0;
}

std::string DigestAuthenticator::get_md5(const std::string& str) {
    uint8_t data[MD5_HASH_SIZE];

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, str.data(), str.size());
    MD5_Final(data, &md5_ctx);

    return to_hex(data, MD5_HASH_SIZE);
}

std::string DigestAuthenticator::to_hex(uint8_t* data, size_t len) {
    std::string ret;
    char const* digits = "0123456789abcdef";
    for (uint8_t* i = data; i < data + len; ++i) {
        ret += digits[((unsigned char) * i) >> 4];
        ret += digits[((unsigned char) * i) & 0xf];
    }
    return ret;
}

std::string DigestAuthenticator::get_response() {
    std::string ha1_hex;
    std::string ha2_hex;

    boost::format ha1_fmt("%s:%s:%s");
    ha1_fmt % user_ % realm_ % pass_;
    ha1_hex = get_md5(ha1_fmt.str());

    boost::format ha2_fmt("%s:%s:%s");
    ha2_fmt % "CONNECT" % host_ % port_;
    ha2_hex = get_md5(ha2_fmt.str());

    nc_ = boost::lexical_cast<std::string>(nonce_count_++);
    nc_.insert(nc_.begin(), 8 - nc_.size(), '0');

    if (cnonce_.empty())
        cnonce_ = get_cnonce(16);

    boost::format response_fmt("%s:%s:%s:%s:%s:%s");
    response_fmt % ha1_hex
    % nonce_
    % nc_
    % cnonce_
    % qop_
    % ha2_hex;

    std::string response_data = response_fmt.str();
    return get_md5(response_data);
}

std::string DigestAuthenticator::get_token() {
    typedef std::map<std::string, std::string> string_map;

    string_map params;
    std::string header;
    response_ = get_response();

    --retries;

    // Build answer parameters
    params["username"] = user_;
    params["realm"] = realm_;
    params["nonce"] = nonce_;
    params["uri"] = host_ + ":" + port_;
    params["response"] = response_;
    params["qop"] = qop_;
    params["cnonce"] = cnonce_;
    params["opaque"] = opaque_;

    header = "Proxy-Authorization: Digest ";

    for (string_map::const_iterator it = params.begin();
            it != params.end(); it++) {
        if (!(it->second.empty())) {
            boost::format fmt("%s=\"%s\"%s");
            fmt % it->first % it->second;
            fmt % ", ";
            header += fmt.str();
        }
    }

    boost::format fmt("nc=%s");
    fmt % nc_;
    header += fmt.str();
    header += "\r\n";

    return header;
}



std::string DigestAuthenticator::get_cnonce(size_t len) {
    const std::string characters("0123456789abcdef");
    boost::uniform_int<> dist(0, characters.size() - 1);
    boost::variate_generator<nonce_rng&, boost::uniform_int<> > urng(rng, dist);

    std::string cnonce;
    for (size_t i = 0; i < len; ++i)
        cnonce += characters.at(urng());
    return cnonce;
}

void DigestAuthenticator::set_headers(const headers_map& headers) {
    Authenticator::set_headers(headers);

    std::string method = headers_["Proxy-Authenticate"];

    qop_ = find_quoted("qop", method);
    nonce_ = find_quoted("nonce", method);
    realm_ = find_quoted("realm", method);
    opaque_ = find_quoted("opaque", method);

    cnonce_.erase();
    // std::cout << method << std::endl;
}

std::string DigestAuthenticator::find_quoted(const std::string& name, const std::string& header) {
    std::string needle = name + "=\"";
    size_t start = header.find(needle);
    if (start != std::string::npos) {
        start += needle.size();
        size_t end = header.find("\"", start);
        return header.substr(start, end - start);
    } else {
        return std::string();
    }
}
}
