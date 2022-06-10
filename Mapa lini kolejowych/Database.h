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

private:
	std::string timestamp;
	SQLite::Database database{ ":memory:" };
	double minlon{1000};
	double minlat{1000};
	double maxlon{};
	double maxlat{};
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

	void splitIntoTiles();
	std::vector<unsigned>& getOccupiedTiles(std::vector<unsigned>& buffer, double _minlon, double _minlat, double _maxlon, double _maxlat);
};

void catchSQLiteException(const SQLite::Exception& e, std::string_view when, std::string_view dump = "");
