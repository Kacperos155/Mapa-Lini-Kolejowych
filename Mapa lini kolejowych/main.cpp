#include "resources.h"
#include "utilities.h"
#include "Server.h"
#include <vector>
#include <string_view>
#include <tinyfiledialogs/tinyfiledialogs.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "iphlpapi.lib")

int main(int argc, char** argv)
{
	/*try
	{
		auto dll = std::filesystem::path("mod_spatialite.dll");
		std::filesystem::create_hard_link("DLL/mod_spatialite.dll", dll);
		std::cerr << std::filesystem::is_other(dll) << '\n';
	}
	catch (std::filesystem::filesystem_error& e)
	{
		std::cerr << e.what() << '\n';
		return 0;
	}*/

	std::vector<std::string_view> arguments(argv, argv + argc);
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
		if (!resources::databaseRebuild(arguments[0]))
			return 1;
	}
	else
	{
		if (!resources::checkDatabaseExistence(arguments[0]))
			return 1;
	}
	
	Server S;
	//std::filesystem::remove("mod_spatialite.dll");
	S.run();
	return 0;
}