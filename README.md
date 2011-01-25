puttle: Tranparent VPN over HTTP Connect
========================================

This tool is inspired by shuttle[1] and use more or less the same concept.


Prerequisites
-------------

 - sudo, su, or logged in as root on your client machine.
   (The server doesn't need admin access.)

   iptables installed on the client, including at
   least the iptables DNAT, REDIRECT, and ttl modules.
   These are installed by default on most Linux distributions.
   (The server doesn't need iptables and doesn't need to be
   Linux.)


How to use it:
--------------

setup-puttle --include w.x.y.x/n --exlude o.t.h.e/r -- puttle \
 --proxy http://user:password@host:port --proxy http://anotherhost:anotherport


[1]: https://github.com/apenwarr/sshuttle
