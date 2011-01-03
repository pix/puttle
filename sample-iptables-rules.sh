#!/bin/bash
#
# Copyright (C) 2011 Camille Moncelier
#
# This file is part of puttle.
#
# puttle is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# puttle is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with puttle in the COPYING.txt file.
# If not, see <http://www.gnu.org/licenses/>.
#

DEST="0.0.0.0/0"
PORT="9090"

[ -n "$2" ] && export DEST="$2"
[ -n "$3" ] && export PORT="$3"

case "$1" in
    on)
        echo "Started forwarding to ${DEST} to local port ${PORT}"
        iptables -t nat -N puttle-${PORT}
        iptables -t nat -F puttle-${PORT}
        iptables -t nat -I OUTPUT 1 -j puttle-${PORT}
        iptables -t nat -I PREROUTING 1 -j puttle-${PORT}
        iptables -t nat -A puttle-${PORT} -j REDIRECT --dest ${DEST} -p tcp --to-ports ${PORT} -m ttl ! --ttl 42
        iptables -t nat -A puttle-${PORT} -j RETURN --dest 127.0.0.0/8 -p tcp
        ;;
    off)
        echo "Stopped to local port ${PORT}"
        iptables -t nat -D OUTPUT -j puttle-${PORT}
        iptables -t nat -D PREROUTING -j puttle-${PORT}
        iptables -t nat -F puttle-${PORT}
        iptables -t nat -X puttle-${PORT}
        ;;
esac

