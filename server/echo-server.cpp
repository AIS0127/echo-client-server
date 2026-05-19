#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#endif // __linux
#ifdef WIN32
#include <ws2tcpip.h>
#endif // WIN32
#include <mutex>
#include <thread>

#ifdef WIN32
void myerror(const char* msg) { fprintf(stderr, "%s %lu\n", msg, GetLastError()); }
#else
void myerror(const char* msg) { fprintf(stderr, "%s %s %d\n", msg, strerror(errno), errno); }
#endif

void usage() {
	printf("echo-server:\n");
	printf("\n");
	printf("syntax: echo-server <port> [-e] [-b]\n");
	printf("sample: echo-server 1234 -e -b\n");
}

struct Param {
	bool broadcast{false};
	bool echo{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc;) {
			if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				i++;
				continue;
			}

			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				i++;
				continue;
			}

			if (argv[i][0] == '-') return false;
			if (i < argc) port = atoi(argv[i++]);
		}
		return port != 0;
	}
} param;

static const int MAX_CLIENTS = 1024;
std::mutex clientMutex;
int clients[MAX_CLIENTS];
int clientCount = 0;

bool addClient(int sd) {
	bool res = false;

	clientMutex.lock();
	if (clientCount < MAX_CLIENTS) {
		clients[clientCount++] = sd;
		res = true;
	}
	clientMutex.unlock();

	return res;
}

void removeClient(int sd) {
	clientMutex.lock();
	for (int i = 0; i < clientCount;) {
		if (clients[i] == sd) {
			for (int j = i; j < clientCount - 1; j++) {
				clients[j] = clients[j + 1];
			}
			clientCount--;
		} else {
			i++;
		}
	}
	clientMutex.unlock();
}

void broadcastMessage(int sender, const char* buf, ssize_t len) {
	clientMutex.lock();
	for (int i = 0; i < clientCount;) {
		int target = clients[i];
		if (target == sender) {
			i++;
			continue;
		}

		ssize_t res = ::send(target, buf, len, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "send return %zd", res);
			myerror(" ");
			for (int j = i; j < clientCount - 1; j++) {
				clients[j] = clients[j + 1];
			}
			clientCount--;
		} else {
			i++;
		}
	}
	clientMutex.unlock();
}

void recvThread(int sd) {
	printf("connected\n");
	fflush(stdout);
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = ::recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "recv return %zd", res);
			myerror(" ");
			break;
		}
		buf[res] = '\0';
		ssize_t len = res;
		printf("%s", buf);
		fflush(stdout);
		if (param.echo) {
			res = ::send(sd, buf, len, 0);
			if (res == 0 || res == -1) {
				fprintf(stderr, "send return %zd", res);
				myerror(" ");
				break;
			}
		}
		if (param.broadcast) {
			broadcastMessage(sd, buf, len);
		}
	}
	printf("disconnected\n");
	fflush(stdout);
	removeClient(sd);
	::close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

#ifdef WIN32
	WSAData wsaData;
	WSAStartup(0x0202, &wsaData);
#endif // WIN32

	//
	// socket
	//
	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		myerror("socket");
		return -1;
	}

#ifdef __linux__
	//
	// setsockopt
	//
	{
		int optval = 1;
		int res = ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
		if (res == -1) {
			myerror("setsockopt");
			return -1;
		}
	}
#endif // __linux

	//
	// bind
	//
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(param.port);

		ssize_t res = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
		if (res == -1) {
			myerror("bind");
			return -1;
		}
	}

	//
	// listen
	//
	{
		int res = listen(sd, 5);
		if (res == -1) {
			myerror("listen");
			return -1;
		}
	}

	while (true) {
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int newsd = ::accept(sd, (struct sockaddr *)&addr, &len);
		if (newsd == -1) {
			myerror("accept");
			break;
		}

		if (!addClient(newsd)) {
			fprintf(stderr, "too many clients\n");
			::close(newsd);
			continue;
		}
		std::thread(recvThread, newsd).detach();
	}
	::close(sd);
}
