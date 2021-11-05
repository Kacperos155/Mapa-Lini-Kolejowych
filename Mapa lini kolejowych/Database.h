#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

class Database
{
public:
	Database();
	Database(std::filesystem::path database_path);
	bool import_from_string(std::string_view rail_lines, std::string_view rail_stations);
	bool import_from_file(std::filesystem::path rail_lines, std::filesystem::path rail_stations);
	void load_from_file(std::filesystem::path database_path);
	void save_to_file(std::filesystem::path database_path);
	const std::string& getGeoJSON(std::string_view polygon_string, int zoom = 20);
private:
	std::string timestamp;
	std::string geoJSON_buffer;
	SQLite::Database database{ ":memory:" };

	bool import_rail_lines(nlohmann::json& data);
	bool import_rail_stations(nlohmann::json& data);
	SQLite::Database create_new_database();
	bool create_temporary_database(nlohmann::json& data);
};
