#include "getElement.h"

void getElement::getData(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, unsigned ID, std::string type)
{
	auto& database = resources::getDatabase();
	auto response = drogon::HttpResponse::newHttpResponse();
	response->setContentTypeString("application/json");
	response->setBody(database.getGeoJSON(ID, type));
	callback(response);
}
