#include "utilities.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>
#include <fmt/core.h>

using string_iterator_pair = std::pair<size_t, size_t>;

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

[[nodiscard]] std::string extractTag(std::string_view page, string_iterator_pair translation_tag)
{
	auto begin = page.begin() + translation_tag.first + 2;
	auto end = page.begin() + translation_tag.second - 2;
	return std::string(begin, end);
}

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

void createTranslationFile(const std::filesystem::path& website_path, const std::filesystem::path& translation_path)
{
	std::stringstream ss;
	{
		std::ifstream website(website_path);
		ss << website.rdbuf();
	}
	auto buffer = ss.str();

	auto tags = findTranslationTags(buffer);
	auto json = nlohmann::json{};
	for (const auto& tag : tags)
	{
		json[extractTag(buffer, tag)] = "";
	}

	std::ofstream output(translation_path);
	output << json.dump(1);
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


	auto tags = findTranslationTags(buffer);
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

void createTranslations(const std::filesystem::path& lang_path, const nlohmann::json& websites)
{
	for (const auto& website : websites)
	{
		auto website_path = website.get<std::string>();
		auto website_translation_file = lang_path / (website_path + ".lang");
		if (std::filesystem::exists(website_translation_file))
		{
			translatePage(website_path, website_translation_file);
		}
		else
		{
			auto orginal_website = lang_path.parent_path() / website_path;
			createTranslationFile(orginal_website, website_translation_file);
			std::filesystem::copy_file(orginal_website, lang_path / website_path);
		}
	}
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
	languages_file >> languages;

	for (const auto& lang : languages["languages"])
	{
		auto& tag = lang["tag"];
		auto tag_dir = directory / tag;
		if (!std::filesystem::exists(tag_dir))
		{
			std::filesystem::create_directories(tag_dir);
			createTranslations(tag_dir, languages["websites"]);
		}
		else
		{
			for (const auto& website : languages["websites"])
			{
				auto website_path = tag_dir / website.get<std::string>();
				auto translation_path = website_path.string() + ".lang";

				if (std::filesystem::exists(translation_path))
				{
					translatePage(website_path, translation_path);
				}
				else
				{
					createTranslationFile(website_path, translation_path);
				}
			}
		}
	}
	return true;
}
