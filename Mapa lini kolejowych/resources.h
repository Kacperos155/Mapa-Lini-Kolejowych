#pragma once
#include "Database.h"
#include <fmt/core.h>
#include <string_view>

namespace resources
{	
	Database& getDatabase();
	const Database& getDatabaseConst();
	bool checkDatabaseExistence(std::string_view app_directory = "");
	bool databaseRebuild(std::string_view app_directory = "");
}
