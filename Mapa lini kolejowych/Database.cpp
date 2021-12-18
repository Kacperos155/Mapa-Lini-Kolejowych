#include "Database.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fstream>
#include <iostream>

#include "SQL/Tables.sql"
#include "SQL/Queries.sql"

Database::Database(const std::filesystem::path& database_path)
{
	loadFromFile(database_path);
}

bool Database::importFromString(std::string_view json_string)
{
	nlohmann::json j;
	j = j.parse(json_string.data());
	auto kb = json_string.size() / 1024;
	auto mb = kb / 1024;
	kb %= 1024;
	fmt::print("Imported string with json data. Size: {}MB {}KB\n", mb, kb);
	return importData(j);
}

bool Database::importFromFile(const std::filesystem::path& json_file)
{
	std::ifstream file(json_file);
	nlohmann::json j;
	file >> j;

	auto file_size = std::filesystem::file_size(json_file);
	auto kb = file_size / 1024;
	auto mb = kb / 1024;
	kb %= 1024;
	fmt::print("Imported {} file. Size: {}MB {}KB\n", json_file.string(), mb, kb);
	return importData(j);
}

void Database::loadFromFile(const std::filesystem::path& database_path)
{
	database = SQLite::Database(database_path.string(), SQLite::OPEN_READONLY);
	database.loadExtension("mod_spatialite.dll", nullptr);
}

void Database::saveToFile(const std::filesystem::path& database_path)
{
	database.backup(database_path.string().c_str(), SQLite::Database::BackupType::Save);
}

std::string Database::find(std::string_view _query, std::string_view type, unsigned limit)
{
	geoJSON_buffer.clear();
	auto query = fmt::format("%{}%", _query);
	nlohmann::json founded;

	auto sql_find = [&](SQLite::Statement& q) {
		q.bind(1, query);
		q.bind(2, limit);

		while (q.executeStep())
		{
			nlohmann::json J;
			J["id"] = q.getColumn("ID").getInt64();
			J["name"] = q.getColumn("Name").getText();
			founded.push_back(J);
		}
	};

	if (type == "rail_station")
	{
		SQLite::Statement station_finder(database, sql::query::search_station);
		sql_find(station_finder);
	}
	else if (type == "rail_line")
	{
		SQLite::Statement line_finder(database, sql::query::search_rail_lines);
		sql_find(line_finder);
	}
	geoJSON_buffer = founded.dump(1);
	return geoJSON_buffer;
}

const std::string& Database::getGeoJSON(unsigned ID, std::string_view type)
{
	geoJSON_buffer.clear();
	if (type == "rail_station")
	{
		SQLite::Statement station_getter(database, sql::query::get_station);
		station_getter.bind(1, ID);
		station_getter.executeStep();

		auto geojson = R"({"type": "Feature", "properties": {}})"_json;
		geojson["properties"]["id"] = ID;
		geojson["properties"]["name"] = station_getter.getColumn("Name").getText();
		geojson["properties"]["type"] = station_getter.getColumn("Type").getText();
		geojson["geometry"] = nlohmann::json::parse(station_getter.getColumn("GeoJson").getText());
		geoJSON_buffer = geojson.dump(1);
	}
	else if (type == "rail_line")
	{
		SQLite::Statement line_getter(database, sql::query::get_rail_line);
		line_getter.bind(1, ID);
		line_getter.executeStep();

		auto geojson = R"({"type": "Feature", "properties": {}})"_json;
		geojson["properties"]["id"] = ID;
		geojson["properties"]["number"] = line_getter.getColumn("Number").getText();
		geojson["properties"]["name"] = line_getter.getColumn("Name").getText();
		geojson["properties"]["network"] = line_getter.getColumn("Network").getText();
		geojson["properties"]["operator"] = line_getter.getColumn("Operator").getText();
		geojson["properties"]["boundry"] = nlohmann::json::parse(line_getter.getColumn("GeoJson").getText());
		auto feature_collection = R"({"type": "FeatureCollection","features": []})"_json;
		{
			SQLite::Statement segment_getter(database, sql::query::get_rail_line_segments);
			segment_getter.bind(1, ID);

			while (segment_getter.executeStep())
			{
				auto lines_geojson = R"({"type": "Feature", "properties": {}})"_json;
				lines_geojson["geometry"] = nlohmann::json::parse(segment_getter.getColumn("GeoJson").getText());
				feature_collection["features"].push_back(std::move(lines_geojson));
			}
		}
		geojson["properties"]["features"] = feature_collection;
		geoJSON_buffer = geojson.dump(1);
	}
	return geoJSON_buffer;
}

const std::string& Database::getGeoJSON(std::string_view polygon_string, int zoom)
{
	geoJSON_buffer.clear();
	auto feature_collection = R"({"type": "FeatureCollection","features": []})"_json;

	//SEGMENTS
	SQLite::Statement segments_intersects(database, sql::query::segments_in_bound);
	segments_intersects.bind(1, polygon_string.data());
	while (segments_intersects.executeStep())
	{
		std::string_view usage = segments_intersects.getColumn("Usage").getText();
		auto max_speed = std::stoi(segments_intersects.getColumn("Max speed").getText());
		auto disusage = segments_intersects.getColumn("Disusage").getInt64();
		auto zoom_restriction = 0;
		if (usage == "main") {
			zoom_restriction = 0;
			if (max_speed < 120) zoom_restriction += 5;
			if (max_speed < 100) zoom_restriction += 1;
		}
		else zoom_restriction = 7;
		if (disusage) zoom_restriction = 8;

		if (zoom - zoom_restriction > 0)
		{
			auto geojson = R"({"type": "Feature", "properties": {}})"_json;
			geojson["properties"]["id"] = segments_intersects.getColumn("ID").getInt64();
			geojson["properties"]["usage"] = usage;
			geojson["properties"]["disusage"] = disusage;
			geojson["properties"]["line"] = segments_intersects.getColumn("Line number").getText();
			geojson["properties"]["maxspeed"] = max_speed;
			geojson["properties"]["electrified"] = segments_intersects.getColumn("Electrified").getInt64();
			geojson["geometry"] = nlohmann::json::parse(segments_intersects.getColumn("GeoJson").getText());
			feature_collection["features"].push_back(std::move(geojson));
		}
	}
	//STATIONS
	if (zoom >= 10)
	{
		SQLite::Statement stations_intersects(database, sql::query::stations_in_bound);
		stations_intersects.bind(1, polygon_string.data());
		while (stations_intersects.executeStep())
		{
			auto type = stations_intersects.getColumn("Type").getInt64();
			auto zoom_restriction = 2;
			if (type == 1) zoom_restriction = 0;
			else if (type >= 4) zoom_restriction = 3;
			if (zoom - zoom_restriction >= 10)
			{
				auto geojson = R"({"type": "Feature", "properties": {}})"_json;
				geojson["properties"]["id"] = stations_intersects.getColumn("ID").getInt64();
				geojson["properties"]["name"] = stations_intersects.getColumn("Name").getText();
				geojson["properties"]["type"] = type;
				geojson["geometry"] = nlohmann::json::parse(stations_intersects.getColumn("GeoJson").getText());
				feature_collection["features"].push_back(std::move(geojson));
			}
		}
	}
	geoJSON_buffer = feature_collection.dump(1);
	return geoJSON_buffer;
}

bool Database::importData(const nlohmann::json& json_data)
{
	database = SQLite::Database(":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
	database.loadExtension("mod_spatialite.dll", nullptr);
	database.exec(sql::tables);
	SQLite::Transaction transaction(database);

	auto timestamp = json_data["osm3s"]["timestamp_osm_base"].get<std::string>();
	std::string type{};
	type.reserve(10);
	int invalid = 0;

	for (const auto& element : json_data["elements"])
	{
		bool r = false;
		element["type"].get_to(type);
		if (type == "way")
		{
			if (!importData_Segment(element))
				++invalid;
		}
		else if (type == "relation")
		{
			if (!importData_RailLine(element))
				++invalid;
		}
		else if (type == "node")
		{
			if (!importData_Station(element))
				++invalid;
		}
	}
	transaction.commit();

	fmt::print("Database created: \n");
	fmt::print("\t Segments: {}\n", database.execAndGet(R"(SELECT COUNT(*) FROM "Segments";)").getText());
	fmt::print("\t Rail lines: {}\n", database.execAndGet(R"(SELECT COUNT(*) FROM "Rail lines";)").getText());
	fmt::print("\t Rail stations: {}\n", database.execAndGet(R"(SELECT COUNT(*) FROM "Rail stations";)").getText());
	if(invalid)
		fmt::print("\t Invalid data: {}\n", invalid);

	return true;
}

bool Database::importData_Segment(const nlohmann::json& json_data)
{
	try
	{
		auto insert_statement = SQLite::Statement{ database, sql::segment_insert };

		static auto id = std::string{};
		static auto boundry = std::string{};
		static auto line = std::string{};
		static auto disusage = std::string{};

		id = json_data["id"].dump();
		insert_statement.bind(":id", id);

		const auto& tags = json_data["tags"];
		bindTag(insert_statement, ":line_number", getTag(tags, "ref"));
		bindTag(insert_statement, ":usage", getTag(tags, "usage"));
		bindTag(insert_statement, ":max_speed", getTag(tags, "maxspeed"));
		bindTag(insert_statement, ":voltage", getTag(tags, "voltage"));

		if (tags.contains("disused:railway") || tags["railway"] == "disused")
			disusage = "disused";
		else if (tags.contains("abandoned:railway"))
			disusage = "abandoned";
		bindTag(insert_statement, ":disusage", disusage.size() ? &disusage : nullptr);
		disusage.clear();

		const auto& bounds = json_data["bounds"];
		boundry = fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
			bounds["minlon"].dump(), bounds["minlat"].dump(),
			bounds["maxlon"].dump(), bounds["maxlat"].dump());

		insert_statement.bind(":boundry", boundry);

		line = "LINESTRING(";
		for (const auto& point : json_data["geometry"])
		{
			line += fmt::format("{} {}, ", point["lon"].dump(), point["lat"].dump());
		}
		line.pop_back();
		line.pop_back();
		line += ")";
		insert_statement.bind(":line", line);

		insert_statement.exec();
		insert_statement.reset();
		return true;
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating rail segment");
		return false;
	}
}

bool Database::importData_RailLine(const nlohmann::json& json_data)
{
	try
	{
		auto insert_statement = SQLite::Statement{ database, sql::rail_line_insert};

		static auto id = std::string{};
		static auto boundry = std::string{};
		static auto line = std::string{};

		id = json_data["id"].dump();
		insert_statement.bind(":id", id);

		const auto& tags = json_data["tags"];
		auto number_ptr = getTag(tags, "ref");
		if (number_ptr == nullptr)
		{
			insert_statement.reset();
			return false;
		}

		bindTag(insert_statement, ":number", number_ptr);
		bindTag(insert_statement, ":name", getTag(tags, "name"));
		bindTag(insert_statement, ":network", getTag(tags, "network"));
		bindTag(insert_statement, ":operator", getTag(tags, "operator"));

		static auto link = SQLite::Statement{ database, sql::rail_line_segment_insert };
		for (const auto& segment : json_data["members"])
		{
			if (segment["type"] != "way")
				continue;
			if (segment["role"] != "")
				continue;
			link.bind(1, id);
			link.bind(2, segment["ref"].dump());

			link.exec();
			link.reset();
		}

		line = "MULTILINESTRING (";
		static auto line_statement = SQLite::Statement{ database, sql::query::get_rail_line_line_strings };
		line_statement.bind(1, id);
		while (line_statement.executeStep())
		{
			auto segment = std::string_view{ line_statement.getColumn(0).getText() };
			segment.remove_prefix(10);
			line += fmt::format("{}, ", segment);
		}
		line_statement.reset();
		if (line.size() < 20)
			return false;

		line.pop_back();
		line.pop_back();
		line += ")";
		insert_statement.bind(":line", line);

		auto boundry_statement = fmt::format("SELECT AsText(Envelope(MultiLineStringFromText(\'{}\', 4326)));", line);
		insert_statement.bind(":boundry", database.execAndGet(boundry_statement).getText());

		insert_statement.exec();
		insert_statement.reset();
		return true;
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating rail line");
		return false;
	}
}

bool Database::importData_Station(const nlohmann::json& json_data)
{
	try
	{
		static auto insert_statement = SQLite::Statement{ database, sql::rail_station_insert };
		static auto id = std::string{};
		static auto point = std::string{};

		id = json_data["id"].dump();
		insert_statement.bind(":id", id);

		const auto& tags = json_data["tags"];
		auto name_ptr = getTag(tags, "name");
		if (name_ptr == nullptr)
		{
			insert_statement.reset();
			return false;
		}

		bindTag(insert_statement, ":name", name_ptr);

		point = fmt::format("POINT({} {})",
			json_data["lon"].dump(), json_data["lat"].dump());
		insert_statement.bind(":point", point);

		unsigned type = 0;
		if (tags.contains("railway"))
		{
			if (tags["railway"] == "station")
				type = 1;
			else if (tags["railway"] == "halt")
				type = 2;
		}
		else if (tags.contains("disused:railway"))
		{
			if (tags["disused:railway"] == "station")
				type = 3;
			else if (tags["disused:railway"] == "halt")
				type = 4;
		}

		insert_statement.bind(":type", type);
		insert_statement.exec();
		insert_statement.reset();
		return true;
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating rail station");
		return false;
	}
}

void Database::catchSQLiteException(const SQLite::Exception& e, std::string_view when, std::string_view dump)
{
	fmt::print(fmt::fg(fmt::color::red), "ERROR: SQL Exception when {}: {}\n", when, e.what());
	if (dump.size())
		fmt::print("Dumping potential exception cause: \n {} \n", dump);
}

const std::string* const Database::getTag(const nlohmann::json& tags, const std::string& tag)
{
	if (tags.contains(tag))
	{
		return &(tags[tag].get_ref<const std::string&>());
	}
	return nullptr;
}
