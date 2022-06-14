#pragma once
#include <SQLiteCpp/Database.h>
#include <filesystem>

namespace utilities
{
	bool loadSpatiaLite(SQLite::Database& database);
	[[nodiscard]] std::string getGeometryBoundry(SQLite::Database& database, std::string_view spatiaLiteGeometry);
	[[nodiscard]] std::string asGeoJSON(SQLite::Database& database, std::string_view spatiaLiteGeometry);
	[[nodiscard]] uint64_t countDatabaseRows(SQLite::Database& database, std::string_view table_name);

	bool translate(const std::filesystem::path& directory);
}
