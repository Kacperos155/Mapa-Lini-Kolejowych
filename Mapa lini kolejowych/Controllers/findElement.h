#pragma once
#include <drogon/HttpController.h>
#include "../resources.h"

class findElement : public drogon::HttpController<findElement>
{
public:
	METHOD_LIST_BEGIN;
	METHOD_ADD(findElement::findData, "{to_find}/{type}", drogon::Get);
	METHOD_ADD(findElement::findData, "{to_find}/{type}/{limit}", drogon::Get);
	METHOD_LIST_END;

private:
	void findData(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string&& query, std::string type = "rail_station", unsigned limit = 5);
};