# 2019/08/01
# flk

CFLAGS = -g -Wall
LIBS = -lpthread -lcrypto
INC = -I.
EXEFILES =   dfile
BINDIR=../bin/

CC = gcc
CPP = g++
RM = rm -rf
MV = mv

OFILES = AppProto.o           \
         Base.o               \
         Connector.o          \
         TcpConnector.o       \
         FileConnector.o      \
         Downloader.o         \
         HttpProto.o          \
         FileProto.o          \
         AppProtoConnector.o  \
         MdAlg.o              \
         Attr.o               \
         Storer.o
MFILE  = Main.o
HFILES = $(patsubst %.o,%.h,$(OFILES))

all     : $(EXEFILES)

.c.o	:
	$(CC) $(CFLAGS) -o $@  -c $< $(INC)
.cpp.o	:
	$(CPP) $(CFLAGS) -o $@  -c $< $(INC)

clean	:
	$(RM) $(EXEFILES) *.o $(BINDIR)


dfile: $(OFILES) $(MFILE) $(HFILES)
	$(CPP) $(CFLAGS) -o $@ $(LIBS) $(MFILE) $(OFILES)

install: $(EXEFILES)
	@rm -f *.o
	@mkdir -p $(BINDIR)
	$(MV) $(EXEFILES) $(BINDIR)

