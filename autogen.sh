#!/bin/sh

set -e
echo -n 'running aclocal.......'
#aclocal-1.6
aclocal
echo 'done'

echo -n 'running libtoolize....'
libtoolize --force
echo 'done'

echo -n 'running autoheader....'
autoheader
echo 'done'

echo -n 'running autoconf......'
autoconf
echo 'done'

echo -n 'running automake......'
#automake-1.6 --copy --foreign --add-missing
automake --copy --foreign --add-missing
echo 'done'

echo 'running configure.....'
#./mips.configure
./configure \
	--prefix=/usr/dot1x \
	--sysconfdir=/etc/eps \
	--with-cgidata=/usr/dot1x/cgi \
	--with-cgibin=/var/epscgi-root \
	--with-openssl=/usr/dot1x \
	--with-openldap=/usr/dot1x \
	--disable-mysql \
	--enable-debug "$@"
echo 'done'
