ACLOCAL_AMFLAGS = -I shave/shave

SUBDIRS = src

if HAVE_DEBUG
	SUBDIRS += tests

lint:
	$(top_srcdir)/vendors/cpplint \
		--filter=-build/header_guard,-whitespace/line_length,-whitespace/labels \
		src/*.cpp src/*.h tests/*.h tests/*.cpp
pretty:
	astyle --brackets=attach \
		src/*.cpp src/*.h tests/*.cpp tests/*.h
endif
