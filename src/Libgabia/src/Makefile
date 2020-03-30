HOME		= /usr/local
INCDIR		= ../inc
PROGRAM = libgabia.a

CCOPT =   -g

INCLUDES = -I$(INCDIR) -I/usr/local/mysql/include
DEFINES = -D_LINUX

CC_FLAGS = 

CFLAGS = $(CCOPT) $(CC_FLAGS) $(DEFINES) $(INCLUDES)

CC = gcc

LINT    = lint

LIBS = -L$(HOME)/lib -leps -L/usr/local/mysql/lib -lmysqlclient

MAKE = make

.c.o:
	$(CC) $(CFLAGS) -c $*.c

#CSRC = sth_syslog.c sth_list.c sth_misc.c sth_string.c sth_mysql.c sth_network.c
CSRC = sth_syslog.c sth_list.c sth_misc.c sth_string.c sth_network.c

SRC =   $(CSRC) $(GEN)

ALLSRC = $(CSRC)

OBJ =   $(SRC:.c=.o)

MAILOBJ = $(MAILSRC:.c=.o)

$(PROGRAM): $(OBJ)
	ar rv $(PROGRAM) $(OBJ)
	ranlib $(PROGRAM)

install : $(PROGRAM)
	cp -f $(PROGRAM) $(HOME)/lib
	mkdir -p /usr/local/include/libgabia
	cp -f ../inc/*.h /usr/local/include/libgabia

test:
	$(CC) $(CFLAGS) $(LIBS) -lmail -lico -o test mail_test.c

clean:
	rm -f $(OBJ) $(PROGRAM)

