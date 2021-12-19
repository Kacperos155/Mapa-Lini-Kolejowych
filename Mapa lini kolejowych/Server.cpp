#include "Server.h"

void Server::run()
{
	auto& app = drogon::app();
	app.addListener("0.0.0.0", 80);
	app.setDocumentRoot("Website/");
	app.enableSession(1800);
	app.registerHandler("/", [](const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback)
		{
			auto response = drogon::HttpResponse::newRedirectionResponse("index.html");
			const auto& lang = req->getCookie("lang");
			
			if (!lang.size())
			{
				response->addCookie("lang", "pl");
			}


			callback(response);

		});
	
	app.registerHandler("/getStyle", [](const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback)
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

		});

	app.run();
}
