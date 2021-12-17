#pragma once
#include "Database.h"
#include <string_view>

namespace resources
{	
	Database& getDatabase();
	const Database& getDatabaseConst();
	bool checkDatabaseExistence(std::string_view app_directory = "");
}

