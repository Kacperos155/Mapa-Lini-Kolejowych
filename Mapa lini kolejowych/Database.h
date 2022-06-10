#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <map>
#include <nlohmann/json.hpp>

#include "BoundingBox.hpp"
#include "Data Types/Database_Types.hpp"

class Database
{
public:
	Database() = default;
	explicit Database(const std::filesystem::path& database_path);
	void showInfo();
	bool importFromString(std::string_view json_string);
	bool importFromFile(const std::filesystem::path& json_file);
	void loadFromFile(const std::filesystem::path& database_path);
	void saveToFile(const std::filesystem::path& database_path);
	const std::string& find(std::string_view query, std::string_view type = "rail_station", unsigned limit = 5);
	const std::string& getGeoJSON(std::string_view ID, std::string_view type = "rail_station");
	const std::string& getGeoJSON(BoundingBox bounds, int zoom = 20);
	std::string getRoute(int64_t start_ID, int64_t end_ID);

	std::string testRoute();

private:
	std::string timestamp;
	SQLite::Database database{ ":memory:" };
	BoundingBox max_bounding{ 1000, 1000, -1000, -1000 };
	std::map<uint64_t, Railnode> railnodes;
	std::map<uint64_t, Railway> railways;
	std::map<uint64_t, Railway_line> raillines;
	std::map<uint64_t, Railway_station> railstations;

	void calcMinMaxBoundry(double _minlon, double _minlat, double _maxlon, double _maxlat);
	bool importData(const nlohmann::json& json_data);
	bool importData_RailNode(const nlohmann::json::array_t& ids, const nlohmann::json::array_t& coords);
	bool importData_Railway(const nlohmann::json& json_data);
	bool importData_RailwayLine(const nlohmann::json& json_data);
	bool importData_RailwayStation(const nlohmann::json& json_data);
};

void catchSQLiteException(const SQLite::Exception& e, std::string_view when, std::string_view dump = "");
