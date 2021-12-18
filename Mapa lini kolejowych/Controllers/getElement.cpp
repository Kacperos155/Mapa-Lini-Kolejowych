#include "getElement.h"

void getElement::getRailStation(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string ID)
{
	getData(req, std::move(callback), ID, "rail_station");
}

void getElement::getRailLine(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string ID)
{
	getData(req, std::move(callback), ID, "rail_line");
}

void getElement::getData(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string ID, std::string type)
{
	auto& database = resources::getDatabase();
	auto response = drogon::HttpResponse::newHttpResponse();
	response->setContentTypeString("application/json");
	response->setBody(database.getGeoJSON(ID, type));
	callback(response);
}
