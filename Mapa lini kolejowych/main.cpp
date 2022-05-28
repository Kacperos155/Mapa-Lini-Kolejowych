#include "resources.h"
#include "utilities.h"
#include "Server.h"
#include <vector>
#include <string_view>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <fmt/core.h>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "iphlpapi.lib")

int main(int argc, char** argv)
{
	auto const application_path = std::filesystem::current_path();

	std::vector<std::string_view> arguments(argv + 1, argv + argc);

	if (std::ranges::find(arguments, "--help") != arguments.end())
	{
		fmt::print("--rebuild-database [--import-json {{json_file}}]\n");
		fmt::print("\tRebuild database with provided file\n");

		fmt::print("--import-json {{json_file}}\n");
		fmt::print("\tOptional argument to provide path to json file\n");

		fmt::print("--translate\n");
		fmt::print("\tTranslate files in Website directory using languages.json file\n");

		fmt::print("--force-console\n");
		fmt::print("\tUse console instead of system dialogs\n");

		fmt::print("\n");
		return 0;
	}

	if (std::ranges::find(arguments, "--force-console") != arguments.end())
	{
		tinyfd_forceConsole = 1;
	}
	if (std::ranges::find(arguments, "--translate") != arguments.end())
	{
		utilities::translate("Website/");
	}
	if (std::ranges::find(arguments, "--rebuild-database") != arguments.end())
	{
		auto import_path = application_path;
		auto import_json_it = std::ranges::find(arguments, "--import-json");

		if (import_json_it != arguments.end())
		{
			if(++import_json_it != arguments.end())
				import_path = *import_json_it;
		}

		if (!resources::databaseRebuild(import_path))
			return 1;
	}
	else
	{
		fmt::print("Use --help to see all commands\n");

		if (!resources::checkDatabaseExistence(application_path))
			return 1;
	}
	
	Server S;
	S.run();
	return 0;
}