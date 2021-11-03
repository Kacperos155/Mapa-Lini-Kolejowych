#include "Database.h"

int main()
{
	try
	{
		Database DB;
		DB.import_from_file("Wynik.json");
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