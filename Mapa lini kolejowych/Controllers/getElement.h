#pragma once
#include <drogon/HttpController.h>
#include "../resources.h"

class getElement : public drogon::HttpController<getElement>
{
public:
	METHOD_LIST_BEGIN;
	METHOD_ADD(getElement::getData, "station/{id}", drogon::Get);
	METHOD_ADD(getElement::getData, "rail_line/{id}", drogon::Get);
	METHOD_LIST_END;

private:
	void getData(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, unsigned ID, std::string type = "rail_station");
};