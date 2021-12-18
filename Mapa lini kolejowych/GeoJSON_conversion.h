#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <nlohmann/json.hpp>
#include <string>

namespace GeoJSON
{
	nlohmann::json getSegment(SQLite::Statement& sql_statement);
	nlohmann::json getRailLine(SQLite::Statement& sql_statement);
	nlohmann::json getRailStation(SQLite::Statement& sql_statement);
	
	nlohmann::json getRailLineLocation(SQLite::Database& database, std::string_view id);
	nlohmann::json getRailStationLocation(SQLite::Database& database, std::string_view id);

	const std::string& allRailLines(SQLite::Database& database, bool refresh = false);
	nlohmann::json& boundingRailLines(SQLite::Database& database, nlohmann::json& features_collection, std::string_view bounds);
	nlohmann::json& boundingMainRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::string_view bounds);
	nlohmann::json& boundingRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::string_view bounds);
}