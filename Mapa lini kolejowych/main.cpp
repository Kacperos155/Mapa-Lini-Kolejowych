#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Rpcrt4.lib")

#include "resources.h"
#include "Server.h"

int main()
{
	auto& DB = resources::getDatabase();
	const std::filesystem::path database_path("Overpass data/Dane kolejowe.db");
	if (std::filesystem::exists(database_path))
		DB.load_from_file(database_path);
	else {
		const std::filesystem::path rail_lines("Overpass data/Linie kolejowe.json");
		const std::filesystem::path rail_stations("Overpass data/Stacje kolejowe.json");
		if (std::filesystem::exists(rail_lines) && std::filesystem::exists(rail_stations))
		{
			if (DB.import_from_file(rail_lines, rail_stations))
				DB.save_to_file(database_path);
			else
			{
				std::cerr << "\nSomething wrong with input files.\n";
				return 1;
			}
		}
		else
		{
			std::cerr << fmt::format("\nYou must provide {} AND {} files.\n", rail_lines.string(), rail_stations.string());
			return 404;
		}
	}
	Server S;
	S.run();
	return 0;
}