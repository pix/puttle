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

#include <map>
#include <string>

using ::puttle::Authenticator;

BOOST_AUTO_TEST_SUITE(authenticator)

BOOST_AUTO_TEST_CASE(basic) {
    Authenticator::pointer auth = Authenticator::create(
                                      Authenticator::AUTH_BASIC,
                                      "Aladdin",
                                      "open sesame",
                                      "192.168.100.1",
                                      "80");

    BOOST_CHECK_EQUAL(auth->has_token(), true);
    BOOST_CHECK_EQUAL(auth->get_token(),
                      "Proxy-Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==\r\n");
}

BOOST_AUTO_TEST_CASE(digest) {
    Authenticator::pointer auth = Authenticator::create(
                                      Authenticator::AUTH_DIGEST,
                                      "Mufasa",
                                      "Circle Of Life",
                                      "192.168.100.1",
                                      "80");

    std::map<std::string, std::string> map_headers;
    map_headers["Proxy-Authenticate"] = "Digest realm=\"testrealm@host.com\", " \
                                        "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", " \
                                        "qop=\"auth\", " \
                                        "stale=false";
    auth->set_headers(map_headers);

    Authenticator::reset_rng();

    BOOST_CHECK_EQUAL(auth->has_token(), true);
    BOOST_CHECK_EQUAL(auth->get_token(),
                      "Proxy-Authorization: Digest cnonce=\"89bd9d8d69a674e0\", " \
                      "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", " \
                      "qop=\"auth\", realm=\"testrealm@host.com\", " \
                      "response=\"723249e2b646bf20ed35cfc004c5d0aa\", " \
                      "uri=\"192.168.100.1:80\", " \
                      "username=\"Mufasa\", " \
                      "nc=00000000\r\n");
}

BOOST_AUTO_TEST_SUITE_END()
