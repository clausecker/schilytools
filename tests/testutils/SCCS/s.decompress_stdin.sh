h26080
s 00003/00001/00001
d D 1.2 15/04/25 22:05:11 joerg 2 1
c gzip -df aufrufen falls uncompress fehlt
e
s 00002/00000/00000
d D 1.1 08/06/19 17:44:22 joerg 1 0
c date and time created 08/06/19 17:44:22 by joerg
e
u
U
f e 0
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
