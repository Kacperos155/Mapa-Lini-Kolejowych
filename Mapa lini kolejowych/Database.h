#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

class Database
{
public:
	Database() = default;
	explicit Database(const std::filesystem::path& database_path);
	bool importFromString(std::string_view json_string);
	bool importFromFile(const std::filesystem::path& json_file);
	void loadFromFile(const std::filesystem::path& database_path);
	void saveToFile(const std::filesystem::path& database_path);
	std::string find(std::string_view query, std::string_view type = "rail_station", unsigned limit = 5);
	const std::string& getGeoJSON(unsigned ID, std::string_view type = "rail_station");
	const std::string& getGeoJSON(std::string_view polygon_string, int zoom = 20);

private:
	std::string timestamp;
	std::string geoJSON_buffer;
	SQLite::Database database{ ":memory:" };

	bool importData(const nlohmann::json& json_data);
	bool importData_Segment(const nlohmann::json& json_data);
	bool importData_RailLine(const nlohmann::json& json_data);
	bool importData_Station(const nlohmann::json& json_data);

	void catchSQLiteException(const SQLite::Exception& e, std::string_view when, std::string_view dump = "");

	[[nodiscard]] const std::string* const getTag(const nlohmann::json& tags, const std::string& tag);
	template<typename T>
	bool bindTag(SQLite::Statement& inserter, std::string_view bind_name, const T* tag);
};

template<typename T>
inline bool Database::bindTag(SQLite::Statement& inserter, std::string_view bind_name, const T* tag)
{
	if (tag != nullptr)
	{
		inserter.bind(bind_name.data(), *tag);
		return true;
	}
	inserter.bind(bind_name.data());
	return false;
}
