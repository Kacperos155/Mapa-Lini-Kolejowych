#include "Database.h"

#include "SQL\Tables.sql"
#include "SQL\Queries.sql"
#include "SQL\Input Tables.sql"

Database::Database()
{
	database.loadExtension("mod_spatialite.dll", nullptr);
	std::cout << database.execAndGet("SELECT spatialite_version();").getText() << '\n';
	std::cout << database.execAndGet("SELECT geos_version();").getText() << '\n';
	std::cout << '\n';
	std::cout << database.execAndGet(R"(SELECT AsGeoJSON(GeomFromText('linestring(15 30, 16 31, 17 32, 18 33, 19 34)', 4326));)").getText() << '\n';
	//std::cout << database.execAndGet(R"(SELECT AsGeoJSON(GeomFromText('POLYGON((52.1320569 20.9288146, 52.2911704 20.9288146, 52.2911704 21.0649742, 52.1320569 21.0649742))', 4326));)").getText() << '\n';
}

bool Database::import_from_string(std::string_view data)
{
	nlohmann::json J(data);
	return importing(J);
}

bool Database::import_from_file(std::filesystem::path file)
{
	std::ifstream f(file);
	nlohmann::json J;
	f >> J;
	return importing(J);
}

bool Database::importing(nlohmann::json& data)
{
	if (!create_temporary_database(data))
		return false;
	try
	{
		database = create_new_database();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what();
		return false;
	}
	database.backup("kek.db", SQLite::Database::BackupType::Save);
	return true;
}

SQLite::Database Database::create_new_database()
{
	SQLite::Database new_database{ ":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE };
	new_database.loadExtension("mod_spatialite.dll", nullptr);

	new_database.exec(sql::tables);
	SQLite::Transaction transaction(new_database);
	SQLite::Statement segments_insert(new_database, sql::segment_insert);
	SQLite::Statement rail_lines_insert(new_database, sql::rail_line_insert);
	SQLite::Statement rail_lines_segments_insert(new_database, sql::rail_line_segment_insert);

	auto ways = SQLite::Statement(database, R"(SELECT * FROM "Ways";)");
	while (ways.executeStep())
	{
		auto id = ways.getColumn("ID").getInt64();
		auto boundry = ways.getColumn("Boundry").getText();
		auto linestring = std::string("LINESTRING(");
		auto query = SQLite::Statement(database, sql::query::ways_coords);
		while (query.executeStep())
		{
			auto lat = query.getColumn("Latitude").getText();
			auto lon = query.getColumn("Longtitude").getText();
			linestring += fmt::format("{} {}, ", lat, lon);
		}
		linestring = linestring.substr(0, linestring.size() - 2);
		linestring += ")";
		segments_insert.bind(":id", id);
		segments_insert.bind(":boundry", boundry);
		segments_insert.bind(":line", linestring);

		segments_insert.exec();
		segments_insert.reset();
	}

	transaction.commit();
	return new_database;
}

bool Database::create_temporary_database(nlohmann::json& data)
{
	const auto timestamp = data["osm3s"]["timestamp_osm_base"].get<std::string>();
	data = data["elements"];

	database.exec(sql::input_tabels);
	SQLite::Transaction transaction(database);
	SQLite::Statement node_insert(database, sql::node_insert);
	SQLite::Statement way_insert(database, sql::way_insert);
	SQLite::Statement relation_insert(database, sql::relation_insert);
	SQLite::Statement ways_nodes_insert(database, sql::way_node_insert);
	SQLite::Statement relations_nodes_insert(database, sql::relation_node_insert);
	SQLite::Statement relations_ways_insert(database, sql::relation_way_insert);

	for (const auto& [index, object] : data.items())
	{
		try
		{
			auto bind_bounds_exec = [](SQLite::Statement& inserter, nlohmann::json& object)
			{
				auto& bounds_json = object["bounds"];
				std::string bounds;
				bounds = fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
					bounds_json["minlat"].dump(), bounds_json["minlon"].dump(),
					bounds_json["maxlat"].dump(), bounds_json["maxlon"].dump());
				inserter.bind(":bounds", bounds);
				inserter.exec();
				inserter.reset();
			};

			auto bind_member_exec = [](SQLite::Statement& inserter, nlohmann::json& value)
			{
				inserter.bind(2, value.dump());
				inserter.exec();
				inserter.reset();
			};

			if (object["type"] == "node")
			{
				node_insert.bind(":id", object["id"].dump());
				node_insert.bind(":lat", object["lat"].dump());
				node_insert.bind(":lon", object["lon"].dump());
				node_insert.exec();
				node_insert.reset();
			}
			else if (object["type"] == "way")
			{
				auto id = object["id"].dump();
				way_insert.bind(":id", object["id"].dump());
				if (object.contains("tags"))
					way_insert.bind(":tags", object["tags"].dump());
				bind_bounds_exec(way_insert, object);

				ways_nodes_insert.bind(1, id);
				for (const auto& [key, value] : object["nodes"].items())
				{
					bind_member_exec(ways_nodes_insert, value);
				}
			}
			else if (object["type"] == "relation")
			{
				auto id = object["id"].dump();

				relation_insert.bind(":id", id);
				if (object.contains("tags"))
					relation_insert.bind(":tags", object["tags"].dump());
				bind_bounds_exec(relation_insert, object);

				relations_ways_insert.bind(1, id);
				relations_nodes_insert.bind(1, id);
				for (const auto& [index, member] : object["members"].items())
				{
					if (member["type"] == "node")
					{
						bind_member_exec(relations_nodes_insert, member["ref"]);
					}
					else if (member["type"] == "way")
					{
						bind_member_exec(relations_ways_insert, member["ref"]);
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
