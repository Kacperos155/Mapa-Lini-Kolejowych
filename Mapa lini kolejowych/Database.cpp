#include "Database.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include <numbers>
#include <queue>
#include <chrono>

#include "GeoJSON_conversion.h"
#include "Router.h"

template<typename T>
const T* bindTag(SQLite::Statement& inserter, std::string_view bind_name, const nlohmann::json& json_object, std::string_view json_key)
{
	if (json_object.contains(json_key))
	{
		inserter.bind(bind_name.data(), json_object[json_key.data()].get_ref<const T&>());
		return json_object[json_key.data()].get_ptr<const T*>();
	}

	inserter.bind(bind_name.data());
	return nullptr;
}

Database::Database(const std::filesystem::path& database_path)
{
	loadFromFile(database_path);
}

void Database::showInfo()
{
	std::map<uint8_t, unsigned> node_connections;
	for (const auto& node : railnodes)
	{
		auto connections = static_cast<uint8_t>(node.second.neighbours.size());
		if (node_connections.contains(connections))
		{
			++node_connections.at(connections);
		}
		else
		{
			node_connections.try_emplace(connections, 1);
		}
	}


	fmt::print("\t Timestamp: {}\n", timestamp);
	fmt::print(fmt::fg(fmt::color::aqua), "\t Rail nodes: {}\n", railnodes.size());
	for (const auto& connection : node_connections)
	{
		fmt::print(fmt::fg(fmt::color::alice_blue), "\t\t With {} connections: {}\n", connection.first, connection.second);
	}

	fmt::print(fmt::fg(fmt::color::aqua), "\t {}: {}\n", Railway::sql_table_name, railways.size());
	fmt::print(fmt::fg(fmt::color::aqua), "\t {}: {}\n", Railway_line::sql_table_name, raillines.size());
	fmt::print(fmt::fg(fmt::color::aqua), "\t {}: {}\n", Railway_station::sql_table_name, railstations.size());

	fmt::print("\t Min Lat: {:.2f}, Min Lon: {:.2f}\n", minlat, minlon);
	fmt::print("\t Max Lat: {:.2f}, Max Lon: {:.2f}\n", maxlat, maxlon);
}

bool Database::importFromString(std::string_view json_string)
{
	nlohmann::json j;
	j = nlohmann::json::parse(json_string.data());
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
	try
	{
		database.loadExtension("mod_spatialite.dll", nullptr);
	}
	catch (const SQLite::Exception& e)
	{
		fmt::print("{}\n", e.what());
	}

	SQLite::Statement infos(database, fmt::format("SELECT * FROM {}", Variable::sql_table_name));
	while (infos.executeStep())
	{
		std::string_view key = infos.getColumn(0).getText();
		const auto& value = infos.getColumn(1);

		if (key == "timestamp")
			timestamp = value.getText();
		else if (key == "minlon")
			minlon = value.getDouble();
		else if (key == "minlat")
			minlat = value.getDouble();
		else if (key == "maxlon")
			maxlon = value.getDouble();
		else if (key == "maxlat")
			maxlat = value.getDouble();
	}
	showInfo();
}

void Database::saveToFile(const std::filesystem::path& database_path)
{
	database.backup(database_path.string().c_str(), SQLite::Database::BackupType::Save);
}

const std::string& Database::find(std::string_view query, std::string_view type, unsigned limit)
{
	static std::string buffer{};
	auto find_query = Railway_station::sql_search;

	if (type == "rail_line")
	{
		find_query = Railway_line::sql_search;
	}
	else if (type != "rail_station")
	{
		throw "Wrong type";
	}

	nlohmann::json j;
	auto search = SQLite::Statement(database, find_query.data());
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

	if (type == "segment")
	{
		buffer = GeoJSON::getSegmentWithBounds(database, ID).dump(1);
	}
	else if (type == "rail_line")
	{
		buffer = GeoJSON::getRailLineWithBounds(database, ID).dump(1);
	}
	else if (type == "rail_station")
	{
		buffer = GeoJSON::getRailStationWithPoint(database, ID).dump(1);
	}
	else
	{
		throw std::logic_error("Wrong type");
	}
	return buffer;
}

const std::string& Database::getGeoJSON(BoundingBox bounds, int zoom)
{
	static std::string buffer{};
	auto features_collection = GeoJSON::createFeatureCollection();

	//Only lines - cache
	if (zoom < 10)
	{
		return GeoJSON::allRailLines(database);
	}
	//Lines + main rail stations
	else if (zoom == 10)
	{
		GeoJSON::boundingRailLines(database, features_collection, bounds);
		GeoJSON::boundingSegments(database, features_collection, bounds);
		GeoJSON::boundingMainRailStations(database, features_collection, bounds);
	}
	//Lines + rail stations
	else if (zoom >= 11)
	{
		GeoJSON::boundingRailLines(database, features_collection, bounds);
		GeoJSON::boundingSegments(database, features_collection, bounds);
		GeoJSON::boundingRailStations(database, features_collection, bounds);
	}

	fmt::print("Elements: {}\n", features_collection["features"].size());
	buffer = features_collection.dump(1);
	return buffer;
}

const std::string Database::getRoute(int64_t start_ID, int64_t end_ID)
{
	static Routing Router{};

	Railnode* start{};
	Railnode* end{};

	auto getNode = [&](Railnode* variable, int64_t ID)
	{
		try
		{
			variable = &railnodes.at(ID);
		}
		catch (std::exception& e)
		{
			fmt::print(fmt::fg(fmt::color::red), "ERROR: There is no node with ID: {}\n", ID);
		}
	};

	getNode(start, start_ID);
	getNode(end, end_ID);
	

	if (Router.route(*start, *end))
	{
		return Router.toGeoJson().dump(1);
	}

	return "";
}

void Database::calcMinMaxBoundry(double _minlon, double _minlat, double _maxlon, double _maxlat)
{
	minlon = std::min(minlon, _minlon);
	minlat = std::min(minlat, _minlat);
	maxlon = std::max(maxlon, _maxlon);
	maxlat = std::max(maxlat, _maxlat);
}

bool Database::importData(const nlohmann::json& json_data)
{
	database = SQLite::Database(":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
	database.loadExtension("mod_spatialite.dll", nullptr);
	SQLite::Transaction transaction(database);

	try
	{
		database.exec(sql_init_spatialite.data());
		database.exec(Railnode::sql_create.data());
		database.exec(Railway::sql_create.data());
		database.exec(Railway_line::sql_create.data());
		database.exec(Railway_station::sql_create.data());
		database.exec(Variable::sql_create.data());
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "recreating tables");
		return false;
	}

	std::string type{};
	type.reserve(10);
	int invalid = 0;

	for (const auto& element : json_data["elements"])
	{
		element["type"].get_to(type);
		if (type == "way")
		{
			if (!importData_Railway(element))
				++invalid;
			else if (element.contains("bounds"))
			{
				const auto& bounds = element["bounds"];
				calcMinMaxBoundry(bounds["minlon"], bounds["minlat"],
					bounds["maxlon"], bounds["maxlat"]);
			}
		}
		else if (type == "relation")
		{
			if (!importData_RailwayLine(element))
				++invalid;
		}
		else if (type == "node")
		{
			if (!importData_RailwayStation(element))
				++invalid;
			else
			{
				const auto lat = element["lat"].get<double>();
				const auto lon = element["lon"].get<double>();
				calcMinMaxBoundry(lon, lat, lon, lat);
			}
		}
	}

	try
	{
		auto insert_statement = SQLite::Statement{ database, Variable::sql_insert.data() };
		auto insert_info = [&insert_statement](std::string_view key, auto value)
		{
			insert_statement.reset();
			insert_statement.bind(1, key.data());
			insert_statement.bind(2, value);
			insert_statement.exec();
		};
		timestamp = json_data["osm3s"]["timestamp_osm_base"].get<std::string>();
		insert_info("timestamp", timestamp);
		insert_info("minlon", minlon);
		insert_info("minlat", minlat);
		insert_info("maxlon", maxlon);
		insert_info("maxlat", maxlat);
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "inserting information");
		return false;
	}

	//splitIntoTiles();
	transaction.commit();

	fmt::print("Database created: \n");
	if (invalid)
		fmt::print("\t Invalid data: {}\n", invalid);
	showInfo();
	return true;
}

bool Database::importData_RailNode(const nlohmann::json::array_t& ids, const nlohmann::json::array_t& coords)
{
	static std::vector<Railnode*> nodes;
	nodes.clear();
	nodes.resize(ids.size());

	for (auto i = 0; i < nodes.size(); ++i)
	{
		auto id = ids[i].get<uint64_t>();

		if (railnodes.contains(id))
		{
			auto& node = railnodes.at(id);
			++node.ref_counter;
		}
		else
		{
			Railnode node;
			node.ID = id;
			node.lat = coords[i]["lat"].get<float>();
			node.lon = coords[i]["lon"].get<float>();
			railnodes.try_emplace(id, std::move(node));
		}
		nodes[i] = &railnodes.at(id);
	}

	for (auto i = 0; i < nodes.size(); ++i)
	{
		if (0 < i)
			nodes[i]->neighbours.push_back(nodes[i - 1]);
		if (i < (nodes.size() - 1))
			nodes[i]->neighbours.push_back(nodes[i + 1]);
	}

	return true;
}

bool Database::importData_Railway(const nlohmann::json& json_data)
{
	try
	{
		Railway railway;
		auto insert_statement = SQLite::Statement{ database, Railway::sql_insert.data() };

		if (auto id = bindTag<int64_t>(insert_statement, ":id", json_data, "id"); id != nullptr)
		{
			railway.ID = *id;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway: ID is empty \n");
			return false;
		}

		const auto& tags = json_data["tags"];

		if (auto line_name = bindTag<std::string>(insert_statement, ":line_name", tags, "ref"); line_name != nullptr)
		{
			railway.line_name = *line_name;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway ({}): Line number is empty \n", railway.ID);
			return false;
		}

		if (auto usage = bindTag<std::string>(insert_statement, ":usage", tags, "usage"); usage != nullptr)
		{
			;// TODO railway.usage = *usage;
		}

		if (auto max_speed = bindTag<std::string>(insert_statement, ":max_speed", tags, "maxspeed"))
		{
			railway.max_speed = static_cast<uint16_t>(std::stoi(*max_speed));
		}

		if (tags.contains("railway"))
		{
			if (tags["railway"] == "disused")
			{
				railway.disusage = true;
			}
		}
		insert_statement.bind(":disusage", railway.disusage);

		if (tags.contains("electrified"))
		{
			if (tags["electrified"] != "no")
			{
				railway.electrified = true;
			}
		}
		insert_statement.bind(":electrified", railway.electrified);


		nlohmann::json::array_t nodes_id = json_data["nodes"];
		nlohmann::json::array_t nodes_coords = json_data["geometry"];
		importData_RailNode(nodes_id, nodes_coords);

		const auto& bounds = json_data["bounds"];
		railway.boundry = fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
			bounds["minlon"].dump(), bounds["minlat"].dump(),
			bounds["maxlon"].dump(), bounds["maxlat"].dump());

		insert_statement.bind(":boundry", railway.boundry);

		railway.line.reserve(16 + 8 * nodes_coords.size());
		railway.line = "LINESTRING(";
		for (const auto& point : nodes_coords)
		{
			railway.line += fmt::format("{} {}, ", point["lon"].dump(), point["lat"].dump());
		}
		railway.line.pop_back();
		railway.line.pop_back();
		railway.line += ")";
		insert_statement.bind(":line", railway.line);

		insert_statement.exec();
		insert_statement.reset();

		railways.try_emplace(railway.ID, std::move(railway));
		return true;
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating railway");
		return false;
	}
}

bool Database::importData_RailwayLine(const nlohmann::json& json_data)
{
	try
	{
		Railway_line rail_line;
		auto insert_statement = SQLite::Statement{ database, Railway_line::sql_insert.data()};

		if (auto id = bindTag<int64_t>(insert_statement, ":id", json_data, "id"); id != nullptr)
		{
			rail_line.ID = *id;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway line: ID is empty \n");
			return false;
		}

		//TODO Assing color to member
		auto id_hash = std::hash<decltype(rail_line.ID)>{}(rail_line.ID);
		auto color = fmt::format("{:x}", id_hash % 200 + 55);
		id_hash /= 255;
		color += fmt::format("{:x}", id_hash % 200 + 55);
		id_hash /= 255;
		color += fmt::format("{:x}", id_hash % 200 + 55);
		insert_statement.bind(":color", color);

		const auto& tags = json_data["tags"];
		if (auto number = bindTag<std::string>(insert_statement, ":number", tags, "ref"); number != nullptr)
		{
			rail_line.number = *number;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway line ({}): Line number is empty \n", rail_line.ID);
			return false;
		}

		if (auto name = bindTag<std::string>(insert_statement, ":name", tags, "name"); name != nullptr)
		{
			rail_line.name = *name;
		}

		if (auto network = bindTag<std::string>(insert_statement, ":network", tags, "network"); network != nullptr)
		{
			rail_line.network = *network;
		}
		
		if (auto line_operator = bindTag<std::string>(insert_statement, ":operator", tags, "operator"); line_operator != nullptr)
		{
			rail_line.line_operator = *line_operator;
		}

		if (auto line_from = bindTag<std::string>(insert_statement, ":from", tags, "from"); line_from != nullptr)
		{
			rail_line.from = *line_from;
		}
		if (auto line_via = bindTag<std::string>(insert_statement, ":via", tags, "via"); line_via != nullptr)
		{
			rail_line.via = *line_via;
		}
		if (auto line_to = bindTag<std::string>(insert_statement, ":to", tags, "to"); line_to != nullptr)
		{
			rail_line.to = *line_to;
		}

		nlohmann::json::array_t segments = json_data["members"];

		uint32_t segments_number{};
		rail_line.line = "MULTILINESTRING (";

		for (const auto& segment : segments)
		{
			if ((segment["type"] != "way") && segment["role"] != "")
				continue;

			auto id = segment["ref"].get<int64_t>();
			if (!railways.contains(id))
				continue;

			++segments_number;
			const auto& railway = railways.at(id);

			std::string_view line_segment = railway.line;
			line_segment.remove_prefix(10);
			rail_line.line += fmt::format("{}, ", line_segment);
		}
		if (!segments_number)
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid rail line ({}): Line has no segments \n", rail_line.ID);
			return false;
		}

		rail_line.line.pop_back();
		rail_line.line.pop_back();
		rail_line.line += ")";
		insert_statement.bind(":line", rail_line.line);

		insert_statement.bind(":boundry", utilities::getGeometryBoundry(database, rail_line.line));

		insert_statement.exec();
		insert_statement.reset();

		raillines.try_emplace(rail_line.ID, std::move(rail_line));
		return true;
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating railway line");
		return false;
	}
}

bool Database::importData_RailwayStation(const nlohmann::json& json_data)
{
	try
	{
		Railway_station station;
		static auto insert_statement = SQLite::Statement{ database, Railway_station::sql_insert.data() };

		if (auto id = bindTag<int64_t>(insert_statement, ":id", json_data, "id"); id != nullptr)
		{
			station.ID = *id;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway line: ID is empty \n");
			return false;
		}

		const auto& tags = json_data["tags"];
		if (auto name = bindTag<std::string>(insert_statement, ":name", tags, "name"); name != nullptr)
		{
			station.name = *name;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway station ({}): Name is empty \n", station.ID);
			return false;
		}

		station.lat = json_data["lat"].get<float>();
		station.lon = json_data["lon"].get<float>();

		station.point = fmt::format("POINT({} {})", station.lon, station.lat);
		insert_statement.bind(":point", station.point);

		station.type = Railway_station::Type::Invalid;
		if (tags.contains("railway"))
		{
			if (tags["railway"] == "station")
				station.type = Railway_station::Type::Station;
			else if (tags["railway"] == "halt")
				station.type = Railway_station::Type::Stop;
		}
		else if (tags.contains("disused:railway"))
		{
			if (tags["disused:railway"] == "station")
				station.type = Railway_station::Type::Disused_Station;
			else if (tags["disused:railway"] == "halt")
				station.type = Railway_station::Type::Disused_Stop;
		}

		if (station.type == Railway_station::Type::Invalid)
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway station ({}): Type is empty \n", station.ID);
			return false;
		}

		insert_statement.bind(":type", static_cast<int>(station.type));
		insert_statement.exec();
		insert_statement.reset();

		railstations.try_emplace(station.ID, std::move(station));
		return true;
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating railway station");
		return false;
	}
}

std::string Database::testRoute()
{
	try
	{
		Routing Router;


		auto time_start = std::chrono::high_resolution_clock::now();

		const auto& start = railnodes.at(9227935317);
		const auto& end = railnodes.at(2895210656);

		if (!Router.route(start, end))
		{
			return {};
		}

		auto time_end = std::chrono::high_resolution_clock::now();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
		auto secounds = milliseconds / 1000;
		milliseconds -= secounds * 1000;
		fmt::print("Time passed on TEST routing: {}s {}ms\n", secounds, milliseconds);

		return Router.toGeoJson().dump(1);
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "creating result of finding connection");
		return {};
	}
}

void Database::splitIntoTiles()
{
	auto distance_lon = std::abs(static_cast<int>(maxlon) - static_cast<int>(minlon));
	auto distance_lat = std::abs(static_cast<int>(maxlat) - static_cast<int>(minlat));

	auto tiles_lon = static_cast<int>(distance_lon) + 1;
	auto tiles_lat = static_cast<int>(distance_lat) + 1;
	auto tiles_number = tiles_lat * tiles_lon;

	auto tiles_for_type = [&](std::string_view input_table)
	{
		std::vector<std::string> table_names;
		table_names.reserve(tiles_number);
		for (int i = 0; i < tiles_number; ++i)
		{
			auto table_name = fmt::format("tile_{}_{}", input_table, i);
			database.exec(fmt::format(R"(DROP TABLE IF EXISTS "{0}"; CREATE TABLE "{0}" ("ID" TEXT PRIMARY KEY);)",
				table_name));
			table_names.push_back(fmt::format(R"(INSERT INTO "{}" VALUES ({{}});)", table_name));
		}
		return table_names;
	};
	std::vector<unsigned> buffer;

	auto split = [&](std::string_view type) {
		auto tables = tiles_for_type(type);
		std::vector<std::pair<double, double>> coords;

		std::string_view statement{};
		if (type == "Rail stations")
			statement = "SELECT ID, asGeoJSON(Point) FROM \"{}\";";
		else
			statement = "SELECT ID, asGeoJSON(Boundry) FROM \"{}\";";

		SQLite::Statement all_id(database, fmt::format(fmt::runtime(statement), type));
		while (all_id.executeStep())
		{
			coords.clear();
			auto id = all_id.getColumn(0).getText();
			auto geojson = nlohmann::json::parse(all_id.getColumn(1).getText());

			if (type == "Rail stations")
			{
				std::pair<double, double> coord = geojson["coordinates"];
				getOccupiedTiles(buffer, coord.first, coord.second, coord.first, coord.second);
			}
			else
			{
				coords = geojson["coordinates"][0];
				getOccupiedTiles(buffer, coords[0].first, coords[0].second, coords[2].first, coords[2].second);
			}

			for (const auto& tile : buffer)
			{
				database.exec(fmt::format(fmt::runtime(tables[tile]), id));
			}
		}
	};
	split(Railway::sql_table_name);
	split(Railway_line::sql_table_name);
	split(Railway_station::sql_table_name);
}

std::vector<unsigned>& Database::getOccupiedTiles(std::vector<unsigned>& buffer, double _minlon, double _minlat, double _maxlon, double _maxlat)
{
	buffer.clear();
	auto tiles_lon = std::abs(static_cast<int>(maxlon) - static_cast<int>(minlon)) + 1;

	_minlon = std::max(_minlon, minlon);
	_minlat = std::max(_minlat, minlat);
	_maxlon = std::min(_maxlon, maxlon);
	_maxlat = std::min(_maxlat, maxlat);

	auto start_lon = std::abs(static_cast<int>(_minlon) - static_cast<int>(minlon));
	auto start_lat = std::abs(static_cast<int>(_minlat) - static_cast<int>(minlat));
	auto end_lon = std::abs(static_cast<int>(_maxlon) - static_cast<int>(minlon));
	auto end_lat = std::abs(static_cast<int>(_maxlat) - static_cast<int>(minlat));

	for (auto y = start_lat; y <= end_lat; ++y)
	{
		for (auto x = start_lon; x <= end_lon; ++x)
		{
			auto i = y * tiles_lon + x;
			buffer.push_back(i);
		}
	}

	return buffer;
}

void catchSQLiteException(const SQLite::Exception& e, std::string_view when, std::string_view dump)
{
	fmt::print(fmt::fg(fmt::color::red), "ERROR: SQL Exception when {}: {}\n", when, e.what());
	if (dump.size())
		fmt::print("Dumping potential exception cause: \n {} \n", dump);
}
