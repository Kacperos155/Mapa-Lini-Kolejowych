#include "GeoJSON_conversion.h"
#include <fmt/core.h>

#include "SQL/GeoJSON.sql"

nlohmann::json GeoJSON::createFeatureCollection()
{
	return R"({"type": "FeatureCollection","features": []})"_json;
}

nlohmann::json GeoJSON::createFeature()
{
	return R"({"type": "Feature", "properties": {}})"_json;
}

template<typename Function>
void fillFeatureCollection(SQLite::Statement& statement, nlohmann::json& features_collection, Function getX)
{
	while (statement.executeStep())
	{
		auto element = getX(statement);
		features_collection["features"].push_back(std::move(element));
	}
}

nlohmann::json getX(SQLite::Statement& sql_statement)
{
	auto obj = GeoJSON::createFeature();
	auto& properties = obj["properties"];
	properties["id"] = sql_statement.getColumn("ID").getText();

	obj["geometry"] = nlohmann::json::parse(sql_statement.getColumn("GeoJson").getText());
	return obj;
}

nlohmann::json GeoJSON::getSegment(SQLite::Statement& sql_statement)
{

	auto obj = getX(sql_statement);
	auto& properties = obj["properties"];

	properties["number"] = sql_statement.getColumn("Line number").getText();

	if (auto column = sql_statement.getColumn("Usage"); !column.isNull())
		properties["usage"] = column.getText();

	if (auto column = sql_statement.getColumn("Max speed"); !column.isNull())
		properties["max_speed"] = column.getText();

	if (auto column = sql_statement.getColumn("Voltage"); !column.isNull())
		properties["voltage"] = column.getText();
	
	if (auto column = sql_statement.getColumn("Disusage"); !column.isNull())
		properties["disusage"] = column.getText();

	return obj;
}

nlohmann::json GeoJSON::getRailLine(SQLite::Statement& sql_statement)
{
	auto obj = getX(sql_statement);
	auto& properties = obj["properties"];

	properties["number"] = sql_statement.getColumn("Number").getText();
	properties["color"] = sql_statement.getColumn("Color").getText();

	if (auto column = sql_statement.getColumn("Name"); !column.isNull())
		properties["name"] = column.getText();
	
	if (auto column = sql_statement.getColumn("Network"); !column.isNull())
		properties["network"] = column.getText();
	
	if (auto column = sql_statement.getColumn("Operator"); !column.isNull())
		properties["operator"] = column.getText();

	return obj;
}

nlohmann::json GeoJSON::getRailStation(SQLite::Statement& sql_statement)
{
	auto obj = getX(sql_statement);
	auto& properties = obj["properties"];

	properties["name"] = sql_statement.getColumn("Name").getText();
	properties["type"] = sql_statement.getColumn("Type").getUInt();

	return obj;
}

nlohmann::json GeoJSON::getSegmentWithBounds(SQLite::Database& database, std::string_view id)
{
	SQLite::Statement getData(database, sql::GeoJson::get_segment);
	getData.bind(1, id.data());
	getData.executeStep();

	auto json = GeoJSON::getSegment(getData);

	json["properties"]["bounds"] = nlohmann::json::parse(getData.getColumn("Boundry").getText());

	return json;
}

nlohmann::json GeoJSON::getRailLineWithBounds(SQLite::Database& database, std::string_view id)
{
	SQLite::Statement getData(database, sql::GeoJson::get_rail_line);
	getData.bind(1, id.data());
	getData.executeStep();

	auto json = GeoJSON::getRailLine(getData);

	json["properties"]["bounds"] = nlohmann::json::parse(getData.getColumn("Boundry").getText());

	return json;
}

nlohmann::json GeoJSON::getRailStationWithPoint(SQLite::Database& database, std::string_view id)
{
	SQLite::Statement getData(database, sql::GeoJson::get_station);
	getData.bind(1, id.data());
	getData.executeStep();

	auto json = GeoJSON::getRailStation(getData);

	return json;
}

const std::string& GeoJSON::allRailLines(SQLite::Database& database, bool refresh)
{
	static std::string buffer{};
	static bool buffered = false;

	if (refresh)
		buffered = false;
	else if (buffered)
		return buffer;

	auto features_collection = createFeatureCollection();
	SQLite::Statement lines(database, sql::GeoJson::all_lines);
	fillFeatureCollection(lines, features_collection, getRailLine);

	buffered = true;
	buffer = features_collection.dump(1);
	return buffer;
}

auto IDsFromTiles(const std::string_view type, std::vector<unsigned>& tiles)
{
	const auto statement_fmt = fmt::format(R"(SELECT ID FROM "tile_{}_{{}}")", type);
	std::string statement = fmt::format(fmt::runtime(statement_fmt), tiles[0]);

	for (int i = 1; i < tiles.size(); ++i)
	{
		statement += fmt::format(" UNION {}", fmt::format(fmt::runtime(statement_fmt), tiles[i]));
	}
	return statement;
}

nlohmann::json& GeoJSON::boundingSegments(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles)
{
	if (tiles.empty())
		return features_collection;

	auto ids = IDsFromTiles("Segments", tiles);
	SQLite::Statement query(database, fmt::format(sql::GeoJson::segments_from_tiles, ids));

	fillFeatureCollection(query, features_collection, getSegment);
	return features_collection;
}

nlohmann::json& GeoJSON::boundingRailLines(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles)
{
	if (tiles.empty())
		return features_collection;

	auto ids = IDsFromTiles("Rail lines", tiles);
	SQLite::Statement query(database, fmt::format(sql::GeoJson::lines_from_tiles, ids));

	fillFeatureCollection(query, features_collection, getRailLine);
	return features_collection;
}

nlohmann::json& GeoJSON::boundingMainRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles)
{
	if (tiles.empty())
		return features_collection;

	auto ids = IDsFromTiles("Rail stations", tiles);
	SQLite::Statement query(database, fmt::format(sql::GeoJson::main_stations_from_tiles, ids));

	fillFeatureCollection(query, features_collection, getRailStation);
	return features_collection;
}

nlohmann::json& GeoJSON::boundingRailStations(SQLite::Database& database, nlohmann::json& features_collection, std::vector<unsigned>& tiles)
{
	if (tiles.empty())
		return features_collection;

	auto ids = IDsFromTiles("Rail stations", tiles);
	SQLite::Statement query(database, fmt::format(sql::GeoJson::stations_from_tiles, ids));

	fillFeatureCollection(query, features_collection, getRailStation);
	return features_collection;
}
