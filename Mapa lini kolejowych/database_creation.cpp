#include "resources.h"
#include "Overpass data/Overpass query.ql"
#include <vector>
#include <string_view>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>

#include <fmt/core.h>
#include <fmt/color.h>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <cpr/cpr.h>

const std::filesystem::path database_path{ "Rail lines database.db" };
const std::filesystem::path OSM_data_path{ "Overpass Data/OSM Rail lines data.json" };
const std::filesystem::path OSM_data_test_path{ "Overpass Data/OSM Rail lines data - pomorskie.json" };

bool databaseFromFiles(std::string_view app_directory)
{
	const char* extension_filter[1] = { "*.json" };
	auto json_data_str = tinyfd_openFileDialog("Rail lines data", app_directory.data(), 1, extension_filter, "JSON file", 0);
	if (json_data_str == nullptr)
		return false;
	auto json_data = std::filesystem::path{ json_data_str };

	if (std::filesystem::exists(json_data))
	{
		auto& DB = resources::getDatabase();
		if (DB.importFromFile(json_data))
		{
			DB.saveToFile(database_path);
			fmt::print("Database succesfuly created from provided file. \n");
			return true;
		}
	}
	fmt::print("Database could not be created from provided files! \n");
	return false;
}

bool databaseFromOSMserver(std::string_view country_tag)
{
	if (country_tag.size() != 2 && country_tag != "test")
	{
		fmt::print(fmt::fg(fmt::color::red), "ERROR: {} is an invalid country tag\n", country_tag);
		return false;
	}

	auto response = cpr::Get(cpr::Url{ "https://overpass-api.de/api/status" });
	if (!cpr::status::is_success(response.status_code))
		return false;
	fmt::print("Overpass API status: \n");
	fmt::print(fmt::fg(fmt::color::yellow), response.text);
	fmt::print("\nPlease be patient. Downloading data could take from 5 to 30 minutes. \n");

	auto query = std::string(overpass_query);
	bool tests = false;
	if (country_tag == "test")
	{
		tests = true;
		query = overpass_query_pomorskie;
	}
	else if (country_tag != "PL")
	{
		auto it = query.find("PL");
		if (it == std::string::npos)
			return false;
		query[it] = country_tag[0];
		query[++it] = country_tag[1];
	}
	query = "https://overpass-api.de/api/interpreter?data=" + cpr::util::urlEncode(query);

	auto download_start = std::chrono::high_resolution_clock::now();
	response = cpr::Get(cpr::Url(std::move(query)));
	auto download_duration = std::chrono::high_resolution_clock::now() - download_start;

	if (!cpr::status::is_success(response.status_code))
	{
		fmt::print(fmt::fg(fmt::color::red), "ERROR: Overpass API returned: {}\n", response.status_code);
		return false;
	}
	fmt::print(fmt::fg(fmt::color::green), "Successful download from Overpass API after {}s\n",
		std::chrono::duration_cast<std::chrono::seconds>(download_duration).count());

	if (!tests)
	{
		std::ofstream output_file(OSM_data_path);
		output_file << response.text;
	}
	else
	{
		std::ofstream output_file(OSM_data_test_path);
		output_file << response.text;
	}

	auto& DB = resources::getDatabase();
	if (DB.importFromString(response.text))
	{
		DB.saveToFile(database_path);
		fmt::print("Database succesfuly created from OpenStreetMap server. \n");
		return true;
	}

	return false;
}

bool resources::checkDatabaseExistence(std::string_view app_directory)
{
	if (std::filesystem::exists(database_path))
		resources::getDatabase().loadFromFile(database_path);
	else if (std::filesystem::exists(OSM_data_path))
	{
		return resources::getDatabase().importFromFile(OSM_data_path);
	}
	else
	{
		return databaseRebuild(app_directory);
	}
	return true;
}

bool resources::databaseRebuild(std::string_view app_directory)
{
	if (tinyfd_messageBox("No database file",
		"There is no database file in application directory. \n" \
		"Do you have a JSON file with rail data? \n\n",
		"yesno", "warning", 0))
	{
		return databaseFromFiles(app_directory);
	}

	if (tinyfd_messageBox("Downloading data from OpenStreetMap server",
		"Do you want to try to download data from OpenStreetMap server? \n\n" \
		"This can take a very long time.",
		"yesno", "warning", 0))
	{
		const char* country_tag = tinyfd_inputBox("Country tag", "Please provide a country name using 2 characters (ISO3166-1:alpha2 standard)", "PL");
		if (country_tag == nullptr)
			return false;

		return databaseFromOSMserver(country_tag);
	}

	fmt::print("Database don't exist and no data was provided to create database. \n");
	return false;
}
