#include "Database.h"

#include "SQL\Tables.sql"
#include "SQL\Queries.sql"
#include "SQL\Input Tables.sql"

Database::Database()
{
	//database.loadExtension("mod_spatialite.dll", nullptr);
	//std::cout << database.execAndGet("SELECT spatialite_version();").getText() << '\n';
	//std::cout << database.execAndGet("SELECT geos_version();").getText() << '\n';
	//std::cout << '\n';
	//std::cout << database.execAndGet(R"(SELECT AsGeoJSON(GeomFromText('linestring(15 30, 16 31, 17 32, 18 33, 19 34)', 4326));)").getText() << '\n';
	////std::cout << database.execAndGet(R"(SELECT AsGeoJSON(GeomFromText('POLYGON((52.1320569 20.9288146, 52.2911704 20.9288146, 52.2911704 21.0649742, 52.1320569 21.0649742))', 4326));)").getText() << '\n';
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

const std::string& Database::getSegments(std::string_view polygon_string)
{
	segments_buffer.clear();
	auto segments = R"({"type": "FeatureCollection","features": []})"_json;

	SQLite::Statement segments_intersects(database, sql::query::segments_in_bound);
	segments_intersects.bind(1, polygon_string.data());
	while (segments_intersects.executeStep())
	{
		auto geojson = R"({"type": "Feature", "properties": {}})"_json;
		geojson["properties"]["id"] = segments_intersects.getColumn("ID").getInt64();
		geojson["geometry"] = nlohmann::json::parse(segments_intersects.getColumn("GeoJson").getText());
		segments["features"].push_back(std::move(geojson));
	}
	segments_buffer = segments.dump(1);
	return segments_buffer;
}

bool Database::importing(nlohmann::json& data)
{
	if (!create_temporary_database(data))
		return false;
	std::cout << "Imported JSON as Database \n";
	fmt::print("Nodes: {}\n", database.execAndGet("SELECT COUNT(*) FROM \"Nodes\"").getInt64());
	fmt::print("Ways: {}\n", database.execAndGet("SELECT COUNT(*) FROM \"Ways\"").getInt64());
	fmt::print("Relations: {}\n", database.execAndGet("SELECT COUNT(*) FROM \"Relations\"").getInt64());
	//database.backup("kek_input.db", SQLite::Database::BackupType::Save);
	try
	{
		database = create_new_database();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what();
		return false;
	}
	std::cout << "Database created \n";
	//database.backup("kek.db", SQLite::Database::BackupType::Save);
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

	auto ways = SQLite::Statement(database, R"(SELECT * FROM "Ways";)");
	while (ways.executeStep())
	{
		auto id = ways.getColumn("ID").getInt64();
		auto boundry = ways.getColumn("Boundry").getText();
		auto points = ways.getColumn("Points").getText();
		segments_insert.bind(":id", id);
		segments_insert.bind(":boundry", boundry);
		segments_insert.bind(":line", points);

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
	SQLite::Statement relations_nodes_insert(database, sql::relation_node_insert);
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

			auto bind_member_exec = [](SQLite::Statement& inserter, nlohmann::json& value)
			{
				inserter.bind(2, value.dump());
				inserter.exec();
				inserter.reset();
			};

			if (object["type"] == "node")
			{
				node_insert.bind(":id", object["id"].dump());
				node_insert.bind(":point", fmt::format("{} {}",
					//object["lat"].dump(), object["lon"].dump()));
					object["lon"].dump(), object["lat"].dump()));
				if (object.contains("tags"))
					way_insert.bind(":tags", object["tags"].dump());
				node_insert.exec();
				node_insert.reset();
			}
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
