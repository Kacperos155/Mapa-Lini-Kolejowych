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
	crow::response& sendData_in_bounds(crow::response& res, double min_lon, double min_lat, double max_lon, double max_lat, int zoom = 20);
};

