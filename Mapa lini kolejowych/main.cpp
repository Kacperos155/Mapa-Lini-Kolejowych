#define CROW_MAIN
#include <crow.h>
#include "Database.h"
#include "Server.h"

int main()
{
	Database DB;
	const std::filesystem::path database_path("Overpass data/Dane kolejowe.db");
	if (std::filesystem::exists(database_path))
		DB.load_from_file(database_path);
	else {
		const std::filesystem::path rail_lines("Overpass data/Linie kolejowe.json");
		const std::filesystem::path rail_station("Overpass data/Stacje kolejowe.json");
		if (std::filesystem::exists(rail_lines) && std::filesystem::exists(rail_station))
		{
			if (DB.import_from_file(rail_lines, rail_station))
				DB.save_to_file(database_path);
			else
			{
				std::cerr << "\nSomething wrong with input files.\n";
				return 1;
			}
		}
		else
		{
			std::cerr << fmt::format("\nYou must provide {} AND {} files.\n", rail_lines.string(), rail_station.string());
			return 404;
		}
	}
	Server S(DB);
	S.run();
	return 0;
}