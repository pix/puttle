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

using ::puttle::Proxy;

BOOST_AUTO_TEST_SUITE(proxy)

BOOST_AUTO_TEST_CASE(basic) {
    Proxy p = Proxy::parse("http://fox:mulder@fbi.gov");

    BOOST_CHECK_EQUAL(p.username, "fox");
    BOOST_CHECK_EQUAL(p.password, "mulder");
    BOOST_CHECK_EQUAL(p.host, "fbi.gov");
    BOOST_CHECK_EQUAL(p.port, 3128);
}

BOOST_AUTO_TEST_CASE(decode) {
    Proxy p = Proxy::parse("http://tom:strange%2F%40%3Dhttp%3A%2F%2Fpasword@fbi.gov");

    BOOST_CHECK_EQUAL(p.username, "tom");
    BOOST_CHECK_EQUAL(p.password, "strange/@=http://pasword");
    BOOST_CHECK_EQUAL(p.host, "fbi.gov");
    BOOST_CHECK_EQUAL(p.port, 3128);
}

BOOST_AUTO_TEST_CASE(borked) {
    Proxy p = Proxy::parse("http://anonymous:%40%%%@fbi.gov.gouv.edu.mil.fr:3129");

    BOOST_CHECK_EQUAL(p.username, "anonymous");
    BOOST_CHECK_EQUAL(p.password, "@%%%");
    BOOST_CHECK_EQUAL(p.host, "fbi.gov.gouv.edu.mil.fr");
    BOOST_CHECK_EQUAL(p.port, 3129);
}
BOOST_AUTO_TEST_SUITE_END()
