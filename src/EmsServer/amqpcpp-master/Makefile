CXX      = g++
CFLAGS   = -Wall -fPIC 
CPPFLAGS = $(CFLAGS) -I/usr/local/include -L/usr/local/lib -Iinclude/

LIBRARIES= rabbitmq
LIBS     = $(addprefix -l,$(LIBRARIES))

LIBNAME  = amqpcpp
LIBFILE  = lib$(LIBNAME).so

SOURCES  = src/AMQP.cpp src/AMQPBase.cpp src/AMQPException.cpp src/AMQPMessage.cpp src/AMQPExchange.cpp src/AMQPQueue.cpp
EXFILES  = example_publish.cpp example_consume.cpp example_get.cpp
EXAMPLES = $(EXFILES:.cpp=)
OBJECTS  = $(SOURCES:.cpp=.o)

all: lib $(EXAMPLES)

lib: $(LIBFILE)

$(LIBFILE): $(OBJECTS)
	@echo $@
	$(CXX) -shared -W1 -o ./lib/$@ $(OBJECTS)

$(EXAMPLES): $(addprefix examples/,$(EXFILES)) $(LIBFILE)
	$(CXX) $(CPPFLAGS) -o $@ examples/$@.cpp -g $(LIBFILE) $(LIBS)


install : 
	cp -f lib/$(LIBFILE) /usr/local/lib

clean:
	rm -f $(OBJECTS) $(EXAMPLES) $(LIBFILE)
