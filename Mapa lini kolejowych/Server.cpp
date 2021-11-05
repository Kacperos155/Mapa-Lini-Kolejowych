#include "Server.h"

Server::Server(Database& database)
	:database(database)
{

}

void Server::run()
{
	init_routing();
	app.port(2137).run();
}

void Server::init_routing()
{
	//Data in bounds
	CROW_ROUTE(app, "/bounds/<double>/<double>/<double>/<double>")
		([this](crow::response& res, double min_lon, double min_lat, double max_lon, double max_lat)
			{
				sendData_in_bounds(res, min_lon, min_lat, max_lon, max_lat).end();
			});
	CROW_ROUTE(app, "/bounds/<double>/<double>/<double>/<double>/<int>")
		([this](crow::response& res, double min_lon, double min_lat, double max_lon, double max_lat, int zoom)
			{
				sendData_in_bounds(res, min_lon, min_lat, max_lon, max_lat, zoom).end();
			});
	//Finding data
	CROW_ROUTE(app, "/find/<string>/<string>")
		([this](crow::response& res, std::string query, std::string type)
			{
				findData(res, query, type).end();
			});
	CROW_ROUTE(app, "/find/<string>/<string>/<uint>")
		([this](crow::response& res, std::string query, std::string type, unsigned limit)
			{
				findData(res, query, type, limit).end();
			});
	//Getting data
	CROW_ROUTE(app, "/get/station/<uint>")
		([this](crow::response& res, unsigned ID)
			{
				getData(res, ID, "rail_station").end();
			});
	CROW_ROUTE(app, "/get/rail_line/<uint>")
		([this](crow::response& res, unsigned ID)
			{
				getData(res, ID, "rail_line").end();
			});
}

crow::response& Server::prepereResponse(crow::response& res)
{
	res.add_header("Access-Control-Allow-Origin", "*");
	res.add_header("Access-Control-Allow-Headers", "Content-Type");
	return res;
}

crow::response& Server::sendData_in_bounds(crow::response& res, double min_lon, double min_lat, double max_lon, double max_lat, int zoom)
{
	auto start = std::chrono::high_resolution_clock::now();
	prepereResponse(res).body = database.getGeoJSON(
		fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
			min_lat, min_lon, max_lat, max_lon), zoom);
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	fmt::print("Request got ready in {} ms\n", duration.count());
	return res;
}

crow::response& Server::findData(crow::response& res, std::string_view query, std::string_view type, unsigned limit)
{
	prepereResponse(res);
	res.body = database.find(query, type, limit);
	return res;
}

crow::response& Server::getData(crow::response& res, unsigned ID, std::string_view type)
{
	prepereResponse(res);
	res.body = database.getGeoJSON(ID, type);
	return res;
}
