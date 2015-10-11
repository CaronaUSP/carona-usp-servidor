bin_PROGRAMS = json_server_demo main ssl_server_demo

json_server_demo_SOURCES = src/programs/json_server_demo.cc src/JSON.cc \
	src/Network.cc
json_server_demo_LDADD = -lssl -lcrypto -lglog

main_SOURCES = src/main.cc src/ClientThread.cc src/JSON.cc src/Network.cc \
	src/Database.cc
main_LDADD = -lssl -lcrypto -lglog -lpthread -lcurl -lsqlite3

ssl_server_demo_SOURCES = src/programs/ssl_server_demo.cc src/Network.cc
ssl_server_demo_LDADD = -lssl -lcrypto -lglog

LISTEN_LIMIT=200

CONFIGURATION = -DSMTP_USER=\"u\" -DSMTP_PASSWORD=\"p\" -DSMTP_ADDRESS=\"a\" \
	-DSMTP_EMAIL=\"e\"

AM_CXXFLAGS = -D_GNU_SOURCE -D_XOPEN_SOURCE=600 -DRAPIDJSON_HAS_STDSTRING \
	-DRAPIDJSON_SSE42 -DLISTEN_LIMIT=$(LISTEN_LIMIT) -I$(srcdir)/src/include \
	-std=c++14 $(CONFIGURATION)
