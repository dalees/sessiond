# Makefile, by Michal Trojnara 2009

VERSION=0.7
NAME=sessiond-$(VERSION)
CPPFLAGS=-O2 -Wall -DVERSION=\"$(VERSION)\"
LDFLAGS=-lstdc++
DSTDIR=/usr/local/bin/
HDRS=data.h log.h
SRCS=sessiond.cpp comm.cpp data.cpp log.cpp
OBJS=sessiond.o comm.o data.o log.o
DOCS=COPYING PROTOCOL README

sessiond: $(OBJS)

sessiond.o: sessiond.cpp Makefile
comm.o: comm.cpp data.h log.h Makefile
data.o: data.cpp data.h Makefile
log.o: log.cpp log.h Makefile

sessiond.exe: $(HDRS) $(SRCS) Makefile
	i586-mingw32msvc-g++ $(CPPFLAGS) -o sessiond.exe -s $(SRCS) -lws2_32

install: sessiond
	install sessiond $(DSTDIR)

install-strip: sessiond
	install -s sessiond $(DSTDIR)

clean:
	rm -f sessiond $(OBJS) sessiond.exe

dist: sessiond.exe
	mkdir $(NAME)
	ln $(DOCS) Makefile ${HDRS} ${SRCS} $(NAME)/
	tar -czf ../$(NAME).tar.gz $(NAME)
	rm -rf $(NAME)
	zip -9 ../$(NAME).zip $(DOCS) sessiond.exe

# end of Makefile
