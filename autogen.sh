rm -f aclocal.m4
rm -f autoconf.h.in
rm -f configure
cp /usr/share/automake/install-sh .
cp /usr/share/automake/missing .
cp /usr/share/automake/mkinstalldirs .
aclocal
autoheader
automake -a
autoconf
rm -rf autom4te.cache/
