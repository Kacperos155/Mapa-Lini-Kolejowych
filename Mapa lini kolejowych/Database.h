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
	bool import_from_string(std::string_view data);
	bool import_from_file(std::filesystem::path file);

	template<typename T>
	static std::string boundryCoords_to_Geom(T& min_lat, T& min_lon, T& max_lat, T& max_lon);
private:
	SQLite::Database database{ ":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE };

	bool importing(nlohmann::json& data);
	SQLite::Database create_new_database();
	bool create_temporary_database(nlohmann::json& data);
};

template<typename T>
inline std::string Database::boundryCoords_to_Geom(T& min_lat, T& min_lon, T& max_lat, T& max_lon)
{
	std::string s;
	s.reserve(100);
	s = fmt::format("GeomFromText(POLYGON({0} {1}, {2} {1}, {2} {3}, {0} {3}))", min_lat, min_lon, max_lat, max_lon);
	return s;
}
