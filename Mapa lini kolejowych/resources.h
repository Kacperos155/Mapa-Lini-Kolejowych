#pragma once
#include "Database.h"
#include <filesystem>

namespace resources
{	
	Database& getDatabase();
	const Database& getDatabaseConst();
	bool checkDatabaseExistence(std::filesystem::path const& app_directory);
	bool databaseRebuild(std::filesystem::path const& import_data);

	static const std::filesystem::path database_path{ "Railway database.db" };
}
