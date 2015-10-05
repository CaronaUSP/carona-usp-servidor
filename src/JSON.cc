#include "JSON.hh"

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstdio>
#include <stdexcept>

namespace JSON {
	SocketStream::SocketStream(Network::Socket& socket) : socket(&socket) {}

	void SocketStream::rewind() {
		pos = 0;
	}

	SocketStream::Ch SocketStream::Peek() const {
		return socket->peek_byte();
	}

	SocketStream::Ch SocketStream::Take() {
		if (pos > 4096)
			throw std::runtime_error("JSON::SocketStream: Huge JSON message");
		pos++;
		return socket->read_byte();
	}

	size_t SocketStream::Tell() {
		return pos;
	}

	SocketStream::Ch* SocketStream::PutBegin() {
		throw std::runtime_error("JSON::SocketStream: Unsupported operation");
	}

	void SocketStream::Put(Ch c) {
		throw std::runtime_error("JSON::SocketStream: Unsupported operation");
	}

	void SocketStream::Flush() {
		throw std::runtime_error("JSON::SocketStream: Unsupported operation");
	}

	size_t SocketStream::PutEnd(Ch* begin) {
		throw std::runtime_error("JSON::SocketStream: Unsupported operation");
	}

	Socket::Socket(int fd) : Network::Socket(fd), socketStream(*this) {}

	bool Socket::next() {
		socketStream.rewind();
		document.ParseStream<rapidjson::kParseStopWhenDoneFlag>(socketStream);
		return document.IsObject();
	}

	void Socket::send(const rapidjson::Document& document) {
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);
		Network::Socket::send(buffer.GetString());
	}

	const rapidjson::Value& Socket::operator[](const std::string& key) const {
		const rapidjson::Value& value = document[key.c_str()];
		if (value.IsNull())
			throw std::runtime_error("Configuration key " + key + " not found");
		return value;
	}

	const rapidjson::Document& Socket::get_document() const {
		return document;
	}

	File::File(const char *filename) : filename(std::string(filename)) {
		load();
	}

	File::File() {}

	File::~File() {
		char *file_buffer = new char[1024 * 1024];
		FILE* file = fopen(filename.c_str(), "w");
		if (file == NULL) {
			perror("fopen");
			throw std::runtime_error("fopen");
		}
		rapidjson::FileWriteStream stream(file, file_buffer, 1024 * 1024);
		rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);
		document->Accept(writer);
		delete[] file_buffer;
		fclose(file);
	}

	const rapidjson::Value& File::operator[](const std::string& key) const {
		const rapidjson::Value& value = (*document)[key.c_str()];
		if (value.IsNull())
			throw std::runtime_error("Configuration key " + key + " not found");
		return value;
	}

	rapidjson::Value& File::operator[](const std::string& key) {
		rapidjson::Value& value = (*document)[key.c_str()];
		if (value.IsNull())
			throw std::runtime_error("Configuration key " + key + " not found");
		return value;
	}

	void File::load() {
		FILE* file = fopen(filename.c_str(), "r");
		if (file == NULL) {
			perror("fopen");
			throw std::runtime_error("fopen");
		}

		char *file_buffer = new char[1024 * 1024];
		document = new rapidjson::Document;
		rapidjson::FileReadStream stream(file, file_buffer, 1024 * 1024);
		document->ParseStream(stream);

		delete[] file_buffer;
		fclose(file);
	}

	rapidjson::Document *GlobalConfig::configuration_document = NULL;
	std::mutex GlobalConfig::connection_count_mutex;

	GlobalConfig::GlobalConfig() {
		if (configuration_document == NULL) {
			filename = "configuration.json";
			load();
			configuration_document = document;
		}
	}

	int GlobalConfig::increment_connections() {
		std::lock_guard<std::mutex> lock(connection_count_mutex);
		rapidjson::Value& conexoes_total = (*document)["conexoes_total"];
		int valor_conexoes_total = conexoes_total.GetInt() + 1;

		conexoes_total = valor_conexoes_total;
		return valor_conexoes_total;
	}
}
