#include "Bounds.h"

void Bounds::sendData_in_bounds(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, double min_lon, double min_lat, double max_lon, double max_lat, int zoom)
{
	auto& database = resources::getDatabase();
	auto start = std::chrono::high_resolution_clock::now();
	auto response = drogon::HttpResponse::newHttpResponse();
	response->setContentTypeString("application/json");

	auto& data = database.getGeoJSON(
		fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))",
			min_lat, min_lon, max_lat, max_lon), zoom);

	response->setBody(data);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	fmt::print("Request got ready in {} ms\n", duration.count());

	callback(response);
}
