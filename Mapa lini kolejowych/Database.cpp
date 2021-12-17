#include "Database.h"

#include "SQL/Tables.sql"
#include "SQL/Queries.sql"
#include "SQL/Input Tables.sql"

Database::Database(const std::filesystem::path& database_path)
{
	load_from_file(database_path);
}

bool Database::import_from_string(std::string_view rail_lines, std::string_view rail_stations)
{
	nlohmann::json L(rail_lines);
	nlohmann::json S(rail_stations);
	if (!import_rail_lines(L))
		return false;
	return import_rail_stations(S);
}

bool Database::import_from_file(const std::filesystem::path& rail_lines, const std::filesystem::path& rail_stations)
{
	std::ifstream fL(rail_lines);
	std::ifstream fS(rail_stations);
	nlohmann::json L;
	nlohmann::json S;
	fL >> L;
	if (!import_rail_lines(L))
		return false;
	fS >> S;
	return import_rail_stations(S);
}

void Database::load_from_file(const std::filesystem::path& database_path)
{
	database = SQLite::Database(database_path.string(), SQLite::OPEN_READONLY);
	database.loadExtension("mod_spatialite.dll", nullptr);
}

void Database::save_to_file(const std::filesystem::path& database_path)
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

bool Database::import_rail_lines(nlohmann::json& data)
{
	if (!create_temporary_database(data))
		return false;
	std::cout << "Imported JSON as Rail Lines Database \n";
	try
	{
		database = create_new_database();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what();
		return false;
	}
	fmt::print("Segments: {}\n", database.execAndGet("SELECT COUNT(*) FROM \"Segments\"").getInt64());
	fmt::print("Rail lines: {}\n", database.execAndGet("SELECT COUNT(*) FROM \"Rail lines\"").getInt64());
	return true;
}

bool Database::import_rail_stations(nlohmann::json& data)
{
	//RAIL STATIONS
	data = data["elements"];
	SQLite::Transaction transaction(database);
	SQLite::Statement rail_station_insert(database, sql::rail_station_insert);
	int i = 0;
	for (const auto& [index, object] : data.items())
	{
		++i;
		auto& tags = object.at("tags");
		rail_station_insert.bind(":id", object.at("id").dump());
		rail_station_insert.bind(":point", fmt::format("POINT({} {})",
			object["lon"].dump(), object["lat"].dump()));
		std::string_view name;
		if (tags.contains("name"))
			name = tags.at("name");
		rail_station_insert.bind(":name", name.data());

		unsigned type = 0;
		if (tags["railway"] == "station")
			type = 1;
		else if (tags["railway"] == "halt")
			type = 2;
		else if (tags["disused:railway"] == "station")
			type = 3;
		else if (tags["disused:railway"] == "halt")
			type = 4;

		rail_station_insert.bind(":type", type);
		rail_station_insert.exec();
		rail_station_insert.reset();
	}
	transaction.commit();
	fmt::print("Rail stations: {}\n", database.execAndGet("SELECT COUNT(*) FROM \"Rail stations\"").getInt64());
	std::cout << "Rail Lines Database is now created \n";
	return true;
}

SQLite::Database Database::create_new_database()
{
	auto ways_i = database.execAndGet("SELECT COUNT(*) FROM \"Ways\"").getInt64();
	long long i = 0;
	SQLite::Database new_database{ ":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE };
	new_database.loadExtension("mod_spatialite.dll", nullptr);

	new_database.exec(sql::tables);
	SQLite::Transaction transaction(new_database);
	SQLite::Statement segments_insert(new_database, sql::segment_insert);
	SQLite::Statement rail_lines_insert(new_database, sql::rail_line_insert);
	SQLite::Statement rail_lines_segments_insert(new_database, sql::rail_line_segment_insert);

	//SEGMENTS
	auto ways = SQLite::Statement(database, R"(SELECT * FROM "Ways";)");
	while (ways.executeStep())
	{
		auto id = ways.getColumn("ID").getInt64();
		auto boundry = ways.getColumn("Boundry").getText();
		auto points = ways.getColumn("Points").getText();
		auto tags = nlohmann::json::parse(ways.getColumn("Tags").getText());

		std::string_view usage;
		std::string_view line_number;
		std::string_view max_speed = "0";
		std::string_view voltage = "0";
		bool disusage = false;
		if (tags.contains("usage"))
			usage = tags.at("usage");
		if (tags.contains("ref"))
			line_number = tags.at("ref");
		if (tags.contains("maxspeed"))
			max_speed = tags.at("maxspeed");
		if (tags.contains("electrified"))
		{
			auto electrified = tags["electrified"].dump();
			if (electrified != "no" && !electrified.empty())
			{
				if (tags.contains("voltage"))
					voltage = tags.at("voltage");
				else
					voltage = "1";
			}
		}
		if (tags.contains("disused:railway") || tags.contains("abandoned:railway") || tags.at("railway") == "disused")
			disusage = true;

		segments_insert.bind(":id", id);
		segments_insert.bind(":boundry", boundry);
		segments_insert.bind(":line", points);
		segments_insert.bind(":usage", usage.data());
		segments_insert.bind(":disusage", disusage);
		segments_insert.bind(":line_number", line_number.data());
		segments_insert.bind(":max_speed", max_speed.data());
		segments_insert.bind(":electrified", voltage.data());

		segments_insert.exec();
		segments_insert.reset();
	}

	//RAIL LINES
	auto relations = SQLite::Statement(database, R"(SELECT * FROM "Relations";)");
	while (relations.executeStep())
	{
		auto id = relations.getColumn("ID").getInt64();
		auto boundry = relations.getColumn("Boundry").getText();
		auto tags = nlohmann::json::parse(relations.getColumn("Tags").getText());

		std::string_view number;
		std::string_view name;
		std::string_view network;
		std::string_view rail_operator;
		if (tags.contains("ref"))
			number = tags.at("ref");
		if (tags.contains("name"))
			name = tags.at("name");
		if (tags.contains("network"))
			network = tags.at("network");
		if (tags.contains("operator"))
			rail_operator = tags.at("operator");

		rail_lines_insert.bind(":id", id);
		rail_lines_insert.bind(":boundry", boundry);
		rail_lines_insert.bind(":number", number.data());
		rail_lines_insert.bind(":name", name.data());
		rail_lines_insert.bind(":network", network.data());
		rail_lines_insert.bind(":operator", rail_operator.data());

		rail_lines_insert.exec();
		rail_lines_insert.reset();
	}
	//RAIL LINES <-> SEGMENTS
	auto link = SQLite::Statement(database, R"(SELECT * FROM "Relations - Ways";)");
	while (link.executeStep())
	{
		rail_lines_segments_insert.bind(1, link.getColumn("Relation_ID").getInt64());
		rail_lines_segments_insert.bind(2, link.getColumn("Way_ID").getInt64());

		rail_lines_segments_insert.exec();
		rail_lines_segments_insert.reset();
	}

	transaction.commit();
	return new_database;
}

bool Database::create_temporary_database(nlohmann::json& data)
{
	database = SQLite::Database(":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
	timestamp = data["osm3s"]["timestamp_osm_base"].get<std::string>();
	data = data["elements"];

	database.exec(sql::input_tabels);
	SQLite::Transaction transaction(database);
	SQLite::Statement node_insert(database, sql::node_insert);
	SQLite::Statement way_insert(database, sql::way_insert);
	SQLite::Statement relation_insert(database, sql::relation_insert);
	SQLite::Statement relations_ways_insert(database, sql::relation_way_insert);

	for (const auto& [index, object] : data.items())
	{
		try
		{
			std::string points;
			points.reserve(100'000);
			auto bind_bounds_exec = [](SQLite::Statement& inserter, nlohmann::json& object)
			{
				auto& bounds_json = object["bounds"];
				std::string bounds;
				bounds = fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
					bounds_json["minlon"].dump(), bounds_json["minlat"].dump(),
					bounds_json["maxlon"].dump(), bounds_json["maxlat"].dump());
				inserter.bind(":bounds", bounds);
				inserter.exec();
				inserter.reset();
			};
			//NODES
			if (object["type"] == "node")
			{
				node_insert.bind(":id", object["id"].dump());
				node_insert.bind(":point", fmt::format("{} {}",
					object["lon"].dump(), object["lat"].dump()));
				if (object.contains("tags"))
					way_insert.bind(":tags", object["tags"].dump());
				node_insert.exec();
				node_insert.reset();
			}
			//WAYS
			else if (object["type"] == "way")
			{
				auto id = object["id"].dump();
				way_insert.bind(":id", object["id"].dump());
				if (object.contains("tags"))
					way_insert.bind(":tags", object["tags"].dump());

				points = "LINESTRING(";
				for (const auto& [key, value] : object["nodes"].items())
				{
					SQLite::Statement point_from_node(database, sql::query::point_from_node);
					point_from_node.bind(1, value.dump());
					point_from_node.executeStep();
					points += point_from_node.getColumn(0).getText();
					points += ", ";
				}
				points = points.substr(0, points.size() - 2);
				points += ")";
				way_insert.bind(":points", points);
				bind_bounds_exec(way_insert, object);
			}
			//RELATIONS
			else if (object["type"] == "relation")
			{
				auto id = object["id"].dump();

				relation_insert.bind(":id", id);
				if (object.contains("tags"))
					relation_insert.bind(":tags", object["tags"].dump());
				bind_bounds_exec(relation_insert, object);

				relations_ways_insert.bind(1, id);
				for (const auto& [index, member] : object["members"].items())
				{
					if (member["type"] == "way")
					{
						relations_ways_insert.bind(2, member["ref"].dump());
						relations_ways_insert.exec();
						relations_ways_insert.reset();
					}
				}
			}
		}
		catch (std::exception e) {
			std::cerr << e.what();
			return false;
		}
	}
	transaction.commit();
	return true;
}
