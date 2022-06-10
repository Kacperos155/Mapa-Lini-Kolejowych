#include "routing.h"

void routing::route(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, int64_t start_ID, int64_t end_ID)
{
	auto& database = resources::getDatabase();
	auto response = drogon::HttpResponse::newHttpResponse();
	response->setContentTypeString("application/json");
	//response->setBody(database.getRoute(start_ID, end_ID));
	response->setBody(database.testRoute());
	callback(response);
}
