hV6,sum=32748
s 00003/00001/00001
d D 1.2 2015/04/25 22:05:11+0200 joerg 2 1
S s 08740
c gzip -df aufrufen falls uncompress fehlt
e
s 00002/00000/00000
d D 1.1 2008/06/19 17:44:22+0200 joerg 1 0
S s 02270
c date and time created 08/06/19 17:44:22 by joerg
e
u
U
f e 0
G r 0e47243586c44
G p tests/testutils/decompress_stdin.sh
t
T
I 1
#!/bin/sh
D 2
exec uncompress
E 2
I 2
type uncompress > /dev/null && exec uncompress
type gzip > /dev/null && exec gzip -df
exit 1
E 2
E 1
