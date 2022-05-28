#include "resources.h"
#include "Overpass data/Overpass query.ql"
#include <vector>
#include <array>
#include <string_view>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>

#include <fmt/core.h>
#include <fmt/color.h>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <cpr/cpr.h>

std::filesystem::path chooseImportFile(std::filesystem::path const& app_directory)
{
	std::array<char const*, 1> extension_filter = { "*.json" };
	auto json_data_str = tinyfd_openFileDialog("Rail lines data", app_directory.string().c_str(), 1, extension_filter.data(), "JSON file", 0);
	if (json_data_str == nullptr)
		return {};
	return { json_data_str };
}

bool databaseFromFile(std::filesystem::path const& import_file)
{
	if (!std::filesystem::exists(import_file))
	{
		fmt::print(fmt::fg(fmt::color::red), "Database could not be created from provided file: {}\n", std::filesystem::absolute(import_file).string());
		return false;
	}

	if (auto& DB = resources::getDatabase(); DB.importFromFile(import_file))
	{
		DB.saveToFile(resources::database_path);
		fmt::print("Database succesfuly created from provided file. \n");
		return true;
	}
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
	fmt::print("Current Overpass API status: \n");
	fmt::print(fmt::fg(fmt::color::yellow), response.text);
	fmt::print("\nPlease be patient. Downloading data could take up to 30 minutes. \n");

	auto query = std::string(overpass_query);
	if (country_tag == "test" || country_tag == "[test]")
	{
		query = overpass_query_pomorskie;
	}
	else if (country_tag != "PL")
	{
		auto it = query.find("PL");
		if (it == std::string::npos)
			return false;
		query[it] = country_tag[0];
		query[it + 1] = country_tag[1];
	}
	query = "https://overpass-api.de/api/interpreter?data=" + cpr::util::urlEncode(query);

	auto output_path = std::filesystem::path{ fmt::format("Overpass data\\{}.json", country_tag) };
	if (!std::filesystem::exists(output_path.parent_path()))
		std::filesystem::create_directories(output_path.parent_path());

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

	std::ofstream output_file(output_path);
	output_file << response.text;

	if (auto& DB = resources::getDatabase(); DB.importFromString(response.text))
	{
		DB.saveToFile(resources::database_path);
		fmt::print("Database succesfuly created from OpenStreetMap server. \n");
		return true;
	}

	return false;
}

bool resources::checkDatabaseExistence(std::filesystem::path const& app_directory)
{
	if (std::filesystem::exists(database_path))
		resources::getDatabase().loadFromFile(database_path);
	else
	{
		return databaseRebuild(app_directory);
	}
	return true;
}

bool resources::databaseRebuild(std::filesystem::path const& import_data)
{
	if (std::filesystem::is_directory(import_data))
	{
		if (tinyfd_messageBox("No database file",
			"There is no database file in application directory. \n" \
			"Do you have a JSON file with rail data? \n\n",
			"yesno", "warning", 0))
		{
			return databaseFromFile(chooseImportFile(import_data));
		}
	}
	else
	{
		return databaseFromFile(import_data);
	}

	if (tinyfd_messageBox("Downloading data from OpenStreetMap server",
		"Do you want to try to download data from OpenStreetMap server? \n\n" \
		"This can take a very long time.",
		"yesno", "warning", 0))
	{
		const char* country_tag = tinyfd_inputBox("Country tag",
			"Please provide a country name using 2 characters (ISO3166-1:alpha2 standard)\nor type [test] to get smaller test data", "PL");
		if (country_tag == nullptr)
			return false;

		return databaseFromOSMserver(country_tag);
	}

	fmt::print("Database don't exist and no data was provided to create database. \n");
	return false;
}
