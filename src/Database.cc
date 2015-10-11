#include "Database.hh"

#include <iostream>
#include <stdexcept>

sqlite3 *Database::database = NULL;

void Database::load(std::string filename) {
	/// @todo Verify advantages of shared cache mode
	if (check_error(sqlite3_open_v2(filename.c_str(), &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
		SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI, NULL))) {
		close();
		throw std::runtime_error("sqlite3_open_v2");
	}
}

void Database::close() {
	if (database != NULL)
		sqlite3_close_v2(database);
}

int Database::check_error(int status) {
	if (status)
		std::cerr << sqlite3_errstr(status) << '\n';
	return status;
}
