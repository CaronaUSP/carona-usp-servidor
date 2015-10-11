#ifndef CARONA_USP_JSON_H_
#define CARONA_USP_JSON_H_

#include <mutex>
#include <rapidjson/document.h>
#include <string>

#include "Network.hh"

namespace JSON {
	class SocketStream {
	public:
		typedef char Ch;
		SocketStream(Network::Socket& socket);
		void rewind();
		Ch Peek() const;
		Ch Take();
		size_t Tell();
		Ch* PutBegin();
		void Put(Ch c);
		void Flush();
		size_t PutEnd(Ch* begin);

	private:
		SocketStream(const SocketStream&);
		SocketStream& operator=(const SocketStream&);
		Network::Socket *socket;
		size_t pos = 0;
	};

	/// @todo Can be done cleanly with inheritation
	class Socket : public Network::Socket {
	public:
		Socket(int fd);
		const rapidjson::Value& operator[](const std::string& key) const;
		bool next();
		void send(const rapidjson::Document& document);
		const rapidjson::Document& get_document() const;

	private:
		rapidjson::Document document;
		SocketStream socketStream;
	};

	/// @todo Can be done cleanly with inheritation
	class File {
	public:
		File(const char *filename);
		File();
		~File();
		const rapidjson::Value& operator[](const std::string& key) const;
		rapidjson::Value& operator[](const std::string& key);

	protected:
		rapidjson::Document *document;
		std::string filename;

		void load();
	};

	class GlobalConfig : public File {
	public:
		GlobalConfig();
		int increment_connections();

	private:
		static std::mutex connection_count_mutex;
		static rapidjson::Document *configuration_document;
	};
}

#endif // CARONA_USP_JSON_H_
