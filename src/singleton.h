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
#ifndef PUTTLE_SRC_SINGLETON_H
#define PUTTLE_SRC_SINGLETON_H

#include <boost/utility.hpp>
#include <boost/thread/once.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>

namespace puttle {

template<class T>
class Singleton : private boost::noncopyable {
public:
    static T& instance() {
        boost::call_once(init_singleton, flag);
        return *t;
    }

protected:
    ~Singleton() {}
    Singleton() {}

    static void init_singleton() {
        t.reset(new T());
    }

private:
    static boost::scoped_ptr<T> t;
    static boost::once_flag flag;
};
}

template<class T> boost::scoped_ptr<T> puttle::Singleton<T>::t(0);
template<class T> boost::once_flag puttle::Singleton<T>::flag = BOOST_ONCE_INIT;

#endif /* end of include guard: PUTTLE_SRC_SINGLETON_H */


