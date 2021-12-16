#include "findElement.h"

void findElement::findData(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string&& query, std::string type, unsigned limit)
{
	auto& database = resources::getDatabase();
	auto response = drogon::HttpResponse::newHttpResponse();
	response->setContentTypeString("application/json");
	response->setBody(database.find(query, type, limit));
	callback(response);
}
