#define CROW_MAIN
#include <crow.h>
#include "Database.h"
#include "Server.h"

int main()
{
	try
	{
		Database DB;
		DB.import_from_file("Wynik - pomorskie.json");
		//DB.import_from_file("Wynik - metro.json");
		Server S(DB);
		S.run();
	}
	catch (SQLite::Exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	catch (nlohmann::json::parse_error& e)
	{
		std::cerr << e.what() << '\n';
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return 0;
}