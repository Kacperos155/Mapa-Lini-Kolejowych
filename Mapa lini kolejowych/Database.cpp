#include "Database.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fstream>
#include <cmath>
#include <chrono>
#include <random>

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
	for (const auto& [id, node] : railnodes)
	{
		auto connections = static_cast<uint8_t>(node.neighbours.size());
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
	for (const auto& [connections, counter] : node_connections)
	{
		fmt::print(fmt::fg(fmt::color::alice_blue), "\t\t With {} connections: {}\n", connections, counter);
	}

	fmt::print(fmt::fg(fmt::color::aqua), "\t {}: {}\n", Railway_station::sql_table_name, railstations.size());

	auto countAndShow = [&](std::string_view table_name)
	{
		fmt::print(fmt::fg(fmt::color::aqua), "\t {}: {}\n", table_name,
			utilities::countDatabaseRows(database, table_name));
	};

	countAndShow(Railway::sql_table_name);
	countAndShow(Railway_line::sql_table_name);

	fmt::print("\t Min Lat: {:.2f}, Min Lon: {:.2f}\n", max_bounding.min_lat, max_bounding.min_lon);
	fmt::print("\t Max Lat: {:.2f}, Max Lon: {:.2f}\n", max_bounding.max_lat, max_bounding.max_lon);
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

bool Database::loadFromFile(const std::filesystem::path& database_path)
{
	database = SQLite::Database(database_path.string(), SQLite::OPEN_READONLY);
	fmt::print("Loading database from \"{}\"\n", database_path.string());
	try
	{
		database.loadExtension("mod_spatialite.dll", nullptr);

		auto max_connections = database.execAndGet(
			fmt::format(R"(SELECT "Value" FROM "{}" WHERE "Key" == "maxNodeConnections")", Variable::sql_table_name)).getInt();

		fmt::print("Loading railnodes... ");
		if (!loadRailnodes(static_cast<uint8_t>(max_connections)))
		{
			fmt::print("Failed\n");
			return false;
		}
		fmt::print("Done\n");

		fmt::print("Loading railway stations... ");
		if (!loadRailwayStations())
		{
			fmt::print("Failed\n");
			return false;
		}
		fmt::print("Done\n");
	}
	catch (const SQLite::Exception& e)
	{
		fmt::print("{}\n", e.what());
		return false;
	}

	SQLite::Statement infos(database, fmt::format("SELECT * FROM {}", Variable::sql_table_name));
	while (infos.executeStep())
	{
		std::string_view key = infos.getColumn(0).getText();
		const auto& value = infos.getColumn(1);

		if (key == "timestamp")
			timestamp = value.getText();
		else if (key == "minlon")
			max_bounding.min_lon = value.getDouble();
		else if (key == "minlat")
			max_bounding.min_lat = value.getDouble();
		else if (key == "maxlon")
			max_bounding.max_lon = value.getDouble();
		else if (key == "maxlat")
			max_bounding.max_lat = value.getDouble();
	}

	fmt::print("Database loaded\n");
	showInfo();
	return true;
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
		throw std::invalid_argument("Wrong type");
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
		throw std::invalid_argument("Wrong type");
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

std::string Database::getRoute(int64_t start_ID, int64_t end_ID)
{
	Railway_station* start{};
	Railway_station* end{};

	auto getNode = [&](Railway_station*& variable, int64_t ID)
	{
		try
		{
			variable = &railstations.at(ID);
		}
		catch (const std::out_of_range& e)
		{
			fmt::print(fmt::fg(fmt::color::red), "ERROR: There is no node with ID: {}\n\t{}\n", ID, e.what());
		}
	};

	getNode(start, start_ID);
	getNode(end, end_ID);

	auto result = route(*start, *end);
	return result.dump(1);
}

void Database::calcMinMaxBoundry(double _minlon, double _minlat, double _maxlon, double _maxlat)
{
	max_bounding.min_lon = std::min(max_bounding.min_lon, _minlon);
	max_bounding.min_lat = std::min(max_bounding.min_lat, _minlat);
	max_bounding.max_lon = std::max(max_bounding.max_lon, _maxlon);
	max_bounding.max_lat = std::max(max_bounding.max_lat, _maxlat);
}

bool Database::importData(const nlohmann::json& json_data)
{
	database = SQLite::Database(":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
	utilities::loadSpatiaLite(database);

	sql_statements.clear();

	SQLite::Transaction transaction(database);
	try
	{
		database.exec(sql_init_spatialite.data());
		database.exec(Railway::sql_create.data());
		database.exec(Railway_line::sql_create.data());
		database.exec(Railway_station::sql_create.data());
		database.exec(Variable::sql_create.data());

		sql_statements.try_emplace(Railway::sql_table_name, database, Railway::sql_insert.data());
		sql_statements.try_emplace(Railway_line::sql_table_name, database, Railway_line::sql_insert.data());
		sql_statements.try_emplace(Railway_station::sql_table_name, database, Railway_station::sql_insert.data());
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
		}
	}
	fmt::print("Inserting railnodes to database... ");

	uint8_t max_connections = 0;
	for (const auto& [ID, node] : railnodes)
	{
		auto connections = static_cast<uint8_t>(node.neighbours.size());
		if (max_connections < connections)
			max_connections = connections;
	}
	if (!insertRailnodes(max_connections))
	{
		fmt::print("Failed\n");
		return false;
	}
	fmt::print("Done\n");

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
		insert_info("minlon", max_bounding.min_lon);
		insert_info("minlat", max_bounding.min_lat);
		insert_info("maxlon", max_bounding.max_lon);
		insert_info("maxlat", max_bounding.max_lat);
		insert_info("maxNodeConnections", max_connections);
			
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "inserting information");
		return false;
	}

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

	for (std::size_t i = 0; i < nodes.size(); ++i)
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
		auto& insert_statement = sql_statements.at(Railway::sql_table_name);

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

		if (auto usage = bindTag<std::string>(insert_statement, ":usage", tags, "usage"); usage != nullptr)
		{
			railway.usage = Railway::Usage::Unknown;// TODO railway.usage = *usage
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


		const nlohmann::json::array_t& nodes_id = json_data["nodes"];
		const nlohmann::json::array_t& nodes_coords = json_data["geometry"];
		importData_RailNode(nodes_id, nodes_coords);

		const auto& bounds = json_data["bounds"];
		railway.boundry = fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
			bounds["minlon"].get<const float>(),
			bounds["minlat"].get<const float>(),
			bounds["maxlon"].get<const float>(),
			bounds["maxlat"].get<const float>()
		);

		insert_statement.bind(":boundry", railway.boundry);

		railway.line.reserve(16 + 8 * nodes_coords.size());
		railway.line = "LINESTRING(";
		for (const auto& point : nodes_coords)
		{
			railway.line +=
				fmt::format("{} {}, ",
					point["lon"].get<const float>(),
					point["lat"].get<const float>()
				);
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
		auto& insert_statement = sql_statements.at(Railway_line::sql_table_name);

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
			auto it = railways.find(id);
			if (it == railways.end())
				continue;

			++segments_number;

			std::string_view line_segment = it->second.line;
			line_segment.remove_prefix(10);
			rail_line.line += fmt::format("{}, ", line_segment);
		}
		if (!segments_number)
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway line ({}): Line \"{}\" has no segments \n", rail_line.ID, rail_line.name);
			return false;
		}

		rail_line.line.pop_back();
		rail_line.line.pop_back();
		rail_line.line += ")";
		insert_statement.bind(":line", rail_line.line);

		insert_statement.bind(":boundry", utilities::getGeometryBoundry(database, rail_line.line));

		insert_statement.exec();
		insert_statement.reset();

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
		auto& insert_statement = sql_statements.at(Railway_station::sql_table_name);

		if (auto id = bindTag<int64_t>(insert_statement, ":id", json_data, "id"); id != nullptr)
		{
			station.ID = *id;
		}
		else
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway station: ID is empty\n");
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
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway station ({}): Name is empty\n", station.ID);
			return false;
		}

		station.lat = json_data["lat"].get<float>();
		station.lon = json_data["lon"].get<float>();

		auto [nearest, distance_to_node] = nearestRailnode(station.lat, station.lon);
		if (distance_to_node > 0.03f)
		{
			insert_statement.reset();
			fmt::print(fmt::fg(fmt::color::orange_red), "Invalid railway station ({}): Station \"{}\" is too far from railway {}\n",
				station.ID, station.name, distance_to_node);
			return false;
		}

		station.node = nearest;
		insert_statement.bind(":node", station.node->ID);

		insert_statement.bind(":lon", station.lon);
		insert_statement.bind(":lat", station.lat);

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

bool Database::loadRailnodes(uint8_t max_connections)
{
	std::vector<std::pair<uint64_t, std::vector<uint64_t>>> railnodes_neighbours{};
	railnodes_neighbours.resize(utilities::countDatabaseRows(database, Railnode::sql_table_name));

	auto query = SQLite::Statement(database,
		fmt::format(fmt::runtime(Railnode::sql_get_all_fmt), getRailnodesConnectionsColumns(max_connections)));

	int index = 0;
	uint8_t max_column_index = max_connections + 2;
	while (query.executeStep())
	{
		Railnode railnode;
		auto& ID = railnodes_neighbours[index].first;
		auto& neighbours = railnodes_neighbours[index].second;
		neighbours.reserve(max_connections);

		ID = std::stoull(query.getColumn(0).getText());

		railnode.ID = ID;
		railnode.lon = static_cast<float>(query.getColumn(max_column_index - 1).getDouble());
		railnode.lat = static_cast<float>(query.getColumn(max_column_index).getDouble());

		for (int i = 1; i < max_column_index - 1; ++i)
		{
			auto connection_column = query.getColumn(i);
			if (connection_column.isNull())
				break;

			neighbours.emplace_back(std::stoull(connection_column.getText()));
		}

		railnodes.try_emplace(ID, std::move(railnode));
		++index;
	}

	for (const auto& [ID, neighbours] : railnodes_neighbours)
	{
		auto& node = railnodes.at(ID);

		for (const auto& n : neighbours)
		{
			node.neighbours.emplace_back(&railnodes.at(n));
		}
	}

	return true;
}

bool Database::loadRailwayStations()
{
	auto query = SQLite::Statement(database, Railway_station::sql_get_all.data());

	while (query.executeStep())
	{
		Railway_station station;

		station.ID = std::stoull(query.getColumn(0).getText());
		station.name = query.getColumn(2).getText();

		station.type = static_cast<Railway_station::Type>(query.getColumn(3).getInt());
		station.lon = static_cast<float>(query.getColumn(4).getDouble());
		station.lat = static_cast<float>(query.getColumn(5).getDouble());

		station.node = &railnodes.at(std::stoull(query.getColumn(1).getText()));

		railstations.try_emplace(station.ID, std::move(station));
	}

	return true;
}

std::string Database::getRailnodesConnectionsColumns(uint8_t max_connections, bool table_create)
{
	if (max_connections == 0)
		throw std::invalid_argument(fmt::format("Max connections of railnode was set only to {}!", max_connections));

	std::string connections;
	for (int i = 1; i <= max_connections; ++i)
	{
		connections += fmt::format("\"Connection_{}\"", i);
		if (table_create)
			connections += "TEXT NULL";
		connections += ", ";
	}
	connections.pop_back();
	connections.pop_back();

	return connections;
}

bool Database::insertRailnodes(uint8_t max_connections)
{
	try
	{
		database.exec(fmt::format(fmt::runtime(Railnode::sql_create_fmt), getRailnodesConnectionsColumns(max_connections, true)));

		std::vector<std::string> connection_binds;
		connection_binds.resize(max_connections);
		std::string insert_sql{};

		for (int i = 0; i < max_connections; ++i)
		{
			connection_binds[i] = fmt::format(":connection_{}", i + 1);
			insert_sql += fmt::format("{}, ", connection_binds[i]);
		}
		insert_sql.pop_back();
		insert_sql.pop_back();
		
		insert_sql = fmt::format(fmt::runtime(Railnode::sql_insert_fmt), getRailnodesConnectionsColumns(max_connections), insert_sql);
		auto insert_statement = SQLite::Statement(database, insert_sql);

		for (const auto& [ID, node] : railnodes)
		{
			insert_statement.bind(":id", static_cast<int64_t>(ID));
			int i = 0;
			for (; i < node.neighbours.size(); ++i)
			{
				insert_statement.bind(connection_binds[i], node.neighbours[i]->ID);
			}
			for (; i < max_connections; ++i)
			{
				insert_statement.bind(connection_binds[i]);
			}

			insert_statement.bind(":lon", node.lon);
			insert_statement.bind(":lat", node.lat);

			insert_statement.exec();
			insert_statement.reset();
		}
	}
	catch (const SQLite::Exception& e)
	{
		catchSQLiteException(e, "inserting railnodes");
		return false;
	}
	return true;
}

std::pair<Railnode*, float> Database::nearestRailnode(float lat, float lon)
{
	Railnode* min_node{};
	auto min_distance = std::numeric_limits<float>::max();

	for (auto& [id, node] : railnodes)
	{
		auto d_lat = node.lat - lat;
		auto d_lon = node.lon - lon;

		d_lat *= d_lat;
		d_lon *= d_lon;

		auto distance = std::sqrt(d_lat + d_lon);

		if (distance < min_distance)
		{
			min_distance = distance;
			min_node = &node;
		}
	}

	return { min_node, min_distance };
}

nlohmann::json Database::route(const Railway_station& start, const Railway_station& end, std::string_view log_prefix) const
{
	Routing Router;

	fmt::print("{} \"{}\" -> \"{}\"\n", log_prefix, start.name, end.name);
	auto is_route_exist = Router.route(*start.node, *end.node);

	auto milliseconds = Router.getTimePassed();
	auto secounds = milliseconds / 1000;
	milliseconds -= secounds * 1000;
	fmt::print("\tDistance: {}\n", Router.getDistance());
	fmt::print("\tTime passed: {}s {}ms\n", secounds, milliseconds);
	fmt::print("\tTraveled nodes: {}\n", Router.getTraveledNodes());

	if (is_route_exist)
		return Router.toGeoJson(start.name, end.name);

	return {};
}

std::string Database::testRoute(int seed)
{
	auto time_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	static std::random_device dev;
	static std::mt19937 rng(static_cast<std::mt19937::result_type>((seed != 0) ? seed : time_seed));
	std::uniform_int_distribution<std::size_t> dist(0, railstations.size() - 2);

	auto start = std::next(railstations.begin(), dist(rng))->second;
	auto end = std::next(railstations.begin(), dist(rng))->second;

	auto result = route(start, end, "TEST Route");
	return result.dump(1);
}

void catchSQLiteException(const SQLite::Exception& e, std::string_view when, std::string_view dump)
{
	fmt::print(fmt::fg(fmt::color::red), "ERROR: SQL Exception when {}: {}\n", when, e.what());
	if (dump.size())
		fmt::print("Dumping potential exception cause: \n {} \n", dump);
}
