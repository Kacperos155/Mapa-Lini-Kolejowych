#pragma once
#include <drogon/HttpController.h>
#include "../resources.h"

class getElement : public drogon::HttpController<getElement>
{
public:
	METHOD_LIST_BEGIN;
	METHOD_ADD(getElement::getRailStation, "rail_station/{id}", drogon::Get);
	METHOD_ADD(getElement::getRailLine, "rail_line/{id}", drogon::Get);
	METHOD_LIST_END;

private:
	void getRailStation(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string ID);
	void getRailLine(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string ID);
	void getData(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, std::string ID, std::string type);
};