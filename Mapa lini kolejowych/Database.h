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
	const std::string& getSegments(std::string_view polygon_string);
private:
	std::string segments_buffer;
	SQLite::Database database{ ":memory:", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE };

	bool importing(nlohmann::json& data);
	SQLite::Database create_new_database();
	bool create_temporary_database(nlohmann::json& data);
};
