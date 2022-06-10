#include "Bounds.h"
#include <fmt/core.h>
#include "../BoundingBox.hpp"

void Bounds::sendData_in_bounds(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, double min_lon, double min_lat, double max_lon, double max_lat, int zoom)
{
	auto& database = resources::getDatabase();
	auto start = std::chrono::high_resolution_clock::now();
	auto response = drogon::HttpResponse::newHttpResponse();
	response->setContentTypeString("application/json");


	auto& data = database.getGeoJSON(BoundingBox(min_lon, min_lat, max_lon, max_lat), zoom);
	response->setBody(data);

	auto duration = std::chrono::high_resolution_clock::now() - start;
	fmt::print("Request got ready in {} ms\n",
		std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());

	callback(response);
}
