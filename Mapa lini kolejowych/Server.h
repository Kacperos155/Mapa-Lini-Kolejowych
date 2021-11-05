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
};

