#include "utilities.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <fmt/color.h>

using string_iterator_pair = std::pair<size_t, size_t>;

void createLanguageFile(const std::filesystem::path& path)
{
	auto json = nlohmann::json{};
	auto lang = nlohmann::json{};

	lang["name"] = "English";
	lang["tag"] = "en";
	json["languages"].push_back(lang);

	lang["name"] = "Polski";
	lang["tag"] = "pl";
	json["languages"].push_back(lang);

	json["websites"].push_back("index.html");

	std::ofstream output(path);
	output << json.dump(1);
}

[[nodiscard]] std::vector<string_iterator_pair> findTranslationTags(std::string_view page)
{
	size_t it = 0;
	std::vector<std::pair<size_t, size_t>> translations;

	while ((it = page.find("!_", it)) != std::string_view::npos)
	{
		auto end_it = page.find("_!", it);
		if (end_it == std::string_view::npos)
			break;

		end_it += 2;
		translations.emplace_back(it, end_it);
		it = end_it;
	}
	return translations;
}

void createTranslationFile(const std::filesystem::path& website_path, const std::filesystem::path& translation_path)
{
	std::stringstream ss;
	{
		std::ifstream website(website_path);
		ss << website.rdbuf();
	}
	auto buffer = ss.str();
	if (buffer.empty())
		return;

	auto tags = findTranslationTags(buffer);
	if (tags.empty())
	{
		std::filesystem::copy_file(website_path, translation_path.parent_path() / website_path.filename(),
			std::filesystem::copy_options::overwrite_existing);
		return;
	}

	auto extractTag = [](std::string_view page, string_iterator_pair translation_tag)
	{
		auto begin = page.begin() + translation_tag.first + 2;
		auto end = page.begin() + translation_tag.second - 2;
		return std::string(begin, end);
	};

	auto json = nlohmann::json{};
	std::string extracted_tag{};
	for (const auto& tag : tags)
	{
		json[extractTag(buffer, tag)] = "";
	}
	if (!json.empty())
	{
		std::ofstream output(translation_path);
		output << json.dump(1);
	}
	fmt::print("Created translation file:{}\n", translation_path.string());
}

void translatePage(const std::filesystem::path& website_path, const std::filesystem::path& translation_path)
{
	auto orginal_website = website_path.parent_path().parent_path() / website_path.filename();
	std::stringstream ss;
	{
		std::ifstream website(orginal_website);
		ss << website.rdbuf();
	}
	auto buffer = ss.str();

	auto translation = nlohmann::json{};
	{
		std::ifstream transl(translation_path);
		transl >> translation;
	}

	std::ofstream output(website_path);
	size_t it = 0;
	size_t start_it = 0;
	std::string tag_name{};

	while ((start_it = buffer.find("!_", it)) != std::string_view::npos)
	{
		output << buffer.substr(it, start_it - it);

		auto end_it = buffer.find("_!", it);
		if (end_it == std::string_view::npos)
			break;

		start_it += 2;
		tag_name = buffer.substr(start_it, end_it - start_it);
		if (translation.contains(tag_name))
		{
			output << translation[tag_name].get<std::string_view>();
		}
		else
		{
			output << tag_name;
		}
		it = end_it + 2;
	}
	output << buffer.substr(it);
	fmt::print("Website {} is translated \n", website_path.string());
}

bool utilities::translate(const std::filesystem::path& directory)
{
	const std::filesystem::path languages_path = "Website/languages.json";
	if (!std::filesystem::exists(languages_path))
	{
		createLanguageFile(languages_path);
	}

	std::ifstream languages_file{ languages_path };
	auto languages = nlohmann::json{};
	try
	{
		languages_file >> languages;
	}
	catch (const nlohmann::json::exception& e)
	{
		fmt::print(fmt::fg(fmt::color::red), "Something wrong with {} file:\n {}\n",
			languages_path.string(), e.what());
		e.what();
		return false;
	}

	for (const auto& lang : languages["languages"])
	{
		if (!lang.contains("tag"))
		{
			fmt::print(fmt::fg(fmt::color::red), "Language don't have a \"tag\" in {} file\n {}\n",
				languages_path.string(), lang.dump(1));
			continue;
		}

		auto& tag = lang["tag"];
		auto tag_dir = directory / tag;
		if (!std::filesystem::exists(tag_dir))
		{
			std::filesystem::create_directories(tag_dir);
		}
		for (const auto& website : languages["websites"])
		{
			const auto& website_str = website.get_ref<const std::string&>();
			auto website_path = tag_dir / website_str;
			auto translation_path = website_path.string() + ".lang";

			if (std::filesystem::exists(translation_path))
			{
				translatePage(website_path, translation_path);
			}
			else
			{
				createTranslationFile(directory / website_str, translation_path);
			}
		}
	}
	return true;
}
