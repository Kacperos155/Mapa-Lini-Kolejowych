#include "Server.h"
#include "fmt/core.h"

void Server::run()
{
	auto& app = drogon::app();
	app.addListener("0.0.0.0", 80);
	app.setDocumentRoot("Website/");
	app.enableSession(1800);
	app.registerHandler("/", [](const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback)
		{
			auto lang = req->getCookie("lang");
			bool no_cookie = false;

			if (!lang.size())
			{
				lang = "pl";
				no_cookie = true;
				
			}

			auto path = fmt::format("{}/index.html", lang);
			auto response = drogon::HttpResponse::newRedirectionResponse(path);
			if(no_cookie)
				response->addCookie("lang", "pl");

			callback(response);

		});
	
	auto getStyle = [](const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback)
	{
		auto response = drogon::HttpResponse::newRedirectionResponse("green.css");
		auto& cookies = req->getCookies();

		if (cookies.contains("style"))
		{
			const auto& style = cookies.at("style");
			if (style == "grey")
				response = response->newRedirectionResponse("grey.css");
		}
		else
		{
			drogon::Cookie cookie("style", "green");
			cookie.setHttpOnly(false);
			response->addCookie(cookie);
		}

		callback(response);

	};

	app.registerHandler("/getStyle", getStyle);
	app.registerHandler("/en/getStyle", getStyle);
	app.registerHandler("/pl/getStyle", getStyle);

	app.run();
}
