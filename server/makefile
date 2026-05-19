TARGET=../bin/echo-server
CPPFLAGS+=-Wall -O2
LDLIBS+=-pthread

all: $(TARGET)

$(TARGET): echo-server.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@


clean:
	rm -f $(TARGET) *.o
