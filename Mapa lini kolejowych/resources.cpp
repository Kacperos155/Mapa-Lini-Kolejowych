#include "resources.h"

namespace
{
	Database database{};
}

Database& resources::getDatabase()
{
	return database;
}

const Database& resources::getDatabaseConst()
{
	return database;
}
