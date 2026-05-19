.PHONY : client server clean

all: client server

client:
	cd client; make; cd ..

server:
	cd server; make; cd ..

clean:
	cd client; make clean; cd ..
	cd server; make clean; cd ..

