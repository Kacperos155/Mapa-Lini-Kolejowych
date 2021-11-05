#pragma once
#include "Database.h"
#include <crow.h>
#include <chrono>

class Server
{
public:
	Server(Database& database);
	void run();

private:
	Database& database;
	crow::SimpleApp app;

	void init_routing();
	crow::response& prepereResponse(crow::response& res);

	crow::response& sendData_in_bounds(crow::response& res, double min_lon, double min_lat, double max_lon, double max_lat, int zoom = 20);
	crow::response& findData(crow::response& res, std::string_view query, std::string_view type = "rail_station", unsigned limit = 5);
	crow::response& getData(crow::response& res, unsigned ID, std::string_view type = "rail_station");
};

