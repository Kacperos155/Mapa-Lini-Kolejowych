#include "GeoJSON_conversion.h"
#include <fmt/core.h>

#include "SQL/GeoJSON.sql"

template<typename Function>
void fillFeatureCollection(SQLite::Statement& statement, nlohmann::json& features_collection, Function getX)
{
	while (statement.executeStep())
	{
		auto element = getX(statement);
		features_collection["features"].push_back(std::move(element));
	}
}

nlohmann::json getLocation(SQLite::Statement&& location, std::string_view id)
{
	auto result = R"({"type": "Feature", "properties": {}})"_json;
	location.bind(1, id.data());
	if (location.executeStep())
	{
		result["properties"]["id"] = id;
		result["geometry"] = nlohmann::json::parse(location.getColumn("GeoJson").getText());
	}
	return result;
}

nlohmann::json GeoJSON::getRailLine(SQLite::Statement& sql_statement)
{
	auto obj = R"({"type": "Feature", "properties": {}})"_json;
	auto& properties = obj["properties"];
	properties["id"] = sql_statement.getColumn("ID").getText();
	properties["number"] = sql_statement.getColumn("Number").getText();

	if (auto column = sql_statement.getColumn("Name"); !column.isNull())
		properties["name"] = column.getText();
	
	if (auto column = sql_statement.getColumn("Network"); !column.isNull())
		properties["network"] = column.getText();
	
	if (auto column = sql_statement.getColumn("Operator"); !column.isNull())
		properties["operator"] = column.getText();

	obj["geometry"] = nlohmann::json::parse(sql_statement.getColumn("GeoJson").getText());

	return obj;
}

nlohmann::json GeoJSON::getRailStation(SQLite::Statement& sql_statement)
{
	auto obj = R"({"type": "Feature", "properties": {}})"_json;
	auto& properties = obj["properties"];
	properties["id"] = sql_statement.getColumn("ID").getText();
	properties["name"] = sql_statement.getColumn("Name").getText();
	properties["type"] = sql_statement.getColumn("Type").getUInt();

	obj["geometry"] = nlohmann::json::parse(sql_statement.getColumn("GeoJson").getText());

	return obj;
}

nlohmann::json GeoJSON::getRailLineLocation(SQLite::Database& database, std::string_view id)
{
	return getLocation(SQLite::Statement(database, sql::GeoJson::get_rail_line_location), id);
}

nlohmann::json GeoJSON::getRailStationLocation(SQLite::Database& database, std::string_view id)
{
	return getLocation(SQLite::Statement(database, sql::GeoJson::get_station_location), id);
}

const std::string& GeoJSON::allRailLines(SQLite::Database& database, bool refresh)
{
	static std::string buffer{};
	static bool buffered = false;

	if (refresh)
		buffered = false;
	else if (buffered)
		return buffer;

	auto features_collection = R"({"type": "FeatureCollection","features": []})"_json;
	SQLite::Statement lines(database, sql::GeoJson::all_lines);
	fillFeatureCollection(lines, features_collection, getRailLine);

	buffered = true;
	buffer = features_collection.dump(1);
	return buffer;
}

nlohmann::json& GeoJSON::boundingRailLines(SQLite::Database& database, nlohmann::json& features_collection, std::string_view bounds)
{
	SQLite::Statement lines(database, sql::GeoJson::lines_in_bound);
	lines.bind(1, bounds.data());
	fillFeatureCollection(lines, features_collection, getRailLine);

	return features_collection;
}

nlohmann::json& GeoJSON::boundingMainRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::string_view bounds)
{
	SQLite::Statement stations(database, sql::GeoJson::main_stations_in_bound);
	stations.bind(1, bounds.data());
	fillFeatureCollection(stations, features_collection, getRailStation);

	return features_collection;
}

nlohmann::json& GeoJSON::boundingRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::string_view bounds)
{
	SQLite::Statement stations(database, sql::GeoJson::stations_in_bound);
	stations.bind(1, bounds.data());
	fillFeatureCollection(stations, features_collection, getRailStation);

	return features_collection;
}
