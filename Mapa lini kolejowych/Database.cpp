#include "Database.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fstream>
#include <iostream>

#include "GeoJSON_conversion.h"
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

const std::string& Database::find(std::string_view query, std::string_view type, unsigned limit)
{
	static std::string buffer{};
	auto find_query = sql::query::search_station;
	auto location_function = GeoJSON::getRailStation;

	if (type == "rail_line")
	{
		find_query = sql::query::search_rail_line;
		location_function = GeoJSON::getRailLine;
	}
	else if (type != "rail_station")
	{
		throw "Wrong type";
	}

	nlohmann::json j;
	auto search = SQLite::Statement(database, find_query);
	search.bind(1, fmt::format("%{}%", query));
	search.bind(2, limit);

	while (search.executeStep())
	{
		nlohmann::json found;
		found["id"] = search.getColumn("ID").getText();
		found["name"] = search.getColumn("Name").getText();
		j.push_back(found);
	}

	buffer = j.dump(1);
	return buffer;
}

const std::string& Database::getGeoJSON(std::string_view ID, std::string_view type)
{
	static std::string buffer{};
	auto location_function = GeoJSON::getRailStationLocation;

	if (type == "rail_line")
	{
		location_function = GeoJSON::getRailLineLocation;
	}
	else if (type != "rail_station")
	{
		throw "Wrong type";
	}

	buffer = location_function(database, ID).dump(1);
	return buffer;
}

const std::string& Database::getGeoJSON(double min_lon, double min_lat, double max_lon, double max_lat, int zoom)
{
	static std::string buffer{};
	auto features_collection = R"({"type": "FeatureCollection","features": []})"_json;
	auto polygon = fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
		min_lon, min_lat, max_lon, max_lat);

	//Only lines - cache
	if (zoom < 8)
	{
		return GeoJSON::allRailLines(database);
	}
	//Only lines - checking boundry
	else if (zoom < 10)
	{
		GeoJSON::boundingRailLines(database, features_collection, polygon);
	}
	//Lines + main rail stations
	else if (zoom == 10)
	{
		GeoJSON::boundingRailLines(database, features_collection, polygon);
		GeoJSON::boundingMainRailStations(database, features_collection, polygon);
	}
	//Lines + rail stations
	else if (zoom >= 11)
	{
		GeoJSON::boundingRailLines(database, features_collection, polygon);
		GeoJSON::boundingRailStations(database, features_collection, polygon);
	}

	buffer = features_collection.dump(1);
	return buffer;
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

		auto boundry_statement = fmt::format("SELECT AsText(Envelope(GeomFromText(\'{}\', 4326)));", line);
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
