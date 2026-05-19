TARGET=../bin/echo-client
CPPFLAGS+=-Wall -O2
LDLIBS+=-pthread

all: $(TARGET)

$(TARGET): echo-client.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@


clean:
	rm -f $(TARGET) *.o
