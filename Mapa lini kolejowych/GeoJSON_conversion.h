#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <nlohmann/json.hpp>
#include <string>

namespace GeoJSON
{
	nlohmann::json createFeatureCollection();
	nlohmann::json createFeature();

	nlohmann::json getSegment(SQLite::Statement& sql_statement);
	nlohmann::json getRailLine(SQLite::Statement& sql_statement);
	nlohmann::json getRailStation(SQLite::Statement& sql_statement);
	
	nlohmann::json getSegmentWithBounds(SQLite::Database& database, std::string_view id);
	nlohmann::json getRailLineWithBounds(SQLite::Database& database, std::string_view id);
	nlohmann::json getRailStationWithPoint(SQLite::Database& database, std::string_view id);

	const std::string& allRailLines(SQLite::Database& database, bool refresh = false);
	
	nlohmann::json& boundingSegments(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles);
	nlohmann::json& boundingRailLines(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles);
	nlohmann::json& boundingMainRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles);
	nlohmann::json& boundingRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles);
}