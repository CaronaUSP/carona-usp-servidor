#ifndef CARONA_USP_DATABASE_H_
#define CARONA_USP_DATABASE_H_

#include <sqlite3.h>

#include <string>

class Database {
public:
	void load(std::string filename);
	void close();

private:
	static sqlite3 *database;

	int check_error(int status);
};

#endif
