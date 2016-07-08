CC=g++
CFLAGS=-Wall -Wno-switch --std=c++11 -L /lib64 -I. -O2
ODIR=obj
LIBS=-lcurl -lncurses -lz -lrt -ltinfo

SRCS=affixes.cpp autoparse.cpp cdnloader.cpp checksum.cpp common.cpp description.cpp file.cpp http.cpp image.cpp imageblp2.cpp imagepng.cpp itemlib.cpp json.cpp logger.cpp miner.cpp ngdp.cpp parser.cpp path.cpp powertag.cpp regexp.cpp snocommon.cpp snomap.cpp strings.cpp texturemgr.cpp translations.cpp utf8.cpp types/Anim.cpp types/AnimSet.cpp types/Default.cpp types/Power.cpp types/SkillKit.cpp types/StringList.cpp types/Textures.cpp

OBJS=$(SRCS:.cpp=.o)

MAIN=autoparse

.PHONY: depend clean

all:	$(MAIN)
	@echo Done

$(MAIN):	$(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS) $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o types/*.o *~ $(MAIN)

