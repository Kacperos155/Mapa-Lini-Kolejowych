#pragma once
#include <drogon/HttpController.h>
#include "../resources.h"

class routing : public drogon::HttpController<routing>
{
public:
	METHOD_LIST_BEGIN;
	METHOD_ADD(routing::test_route, "", drogon::Get);
	METHOD_ADD(routing::route, "{Start_ID}/{End_ID}", drogon::Get);
	METHOD_LIST_END;

private:
	void test_route(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback);
	void route(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, int64_t start_ID, int64_t end_ID);
};