lib_LTLIBRARIES = libcmn.la
libcmn_la_CFLAGS = -DREGEX_MALLOC=1
libcmn_la_LIBADD =
libcmn_la_SOURCES = \
	md4.c md5.c sha1.c \
	uuid.c des.c \
	base64.c \
	url.c url.h \
	hmac.c \
	regex.c regex.h \
	isaac.c isaac.h \
	log.c rbtree.c \
	argcv.c \
	getline.c vector.c
INCLUDES = \
	-I$(top_srcdir)/include -I.
EXTRA_DIST = \
	cmn.dsp
