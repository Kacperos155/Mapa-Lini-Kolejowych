#include "resources.h"
#include "Server.h"
#include <vector>
#include <string_view>
#include <fmt/core.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Rpcrt4.lib")

int main(int argc, char** argv)
{
	std::vector<std::string_view> arguments(argv, argv + argc);
	if (std::ranges::find(arguments, "--force-console") != arguments.end())
	{
		tinyfd_forceConsole = 1;
	}

	if (!resources::checkDatabaseExistence(arguments[0]))
		return 1;
	
	Server S;
	//S.run();
	return 0;
}