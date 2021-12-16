#pragma once
#include <drogon/HttpController.h>
#include "../resources.h"

class Bounds : public drogon::HttpController<Bounds>
{
public:
	METHOD_LIST_BEGIN;
	METHOD_ADD(Bounds::sendData_in_bounds, "{}/{}/{}/{}", drogon::Get);
	METHOD_ADD(Bounds::sendData_in_bounds, "{}/{}/{}/{}/{zoom}", drogon::Get);
	METHOD_LIST_END;

private:
	void sendData_in_bounds(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, double min_lon, double min_lat, double max_lon, double max_lat, int zoom = 20);
};