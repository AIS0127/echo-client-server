TEMPLATE = subdirs
SUBDIRS += \
	client \
	server

client.file = client/echo-client.pro
server.file = server/echo-server.pro
