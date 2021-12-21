#include "Server.h"
#include "fmt/core.h"
#include "fmt/color.h"

void getTranslation(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback, const std::string& path)
{
	auto& cookies = req->getCookies();
	std::string_view lang = "pl";
	bool no_cookie = false;

	if (cookies.contains("lang"))
	{
		lang = cookies.at("lang");
	}
	else
	{
		no_cookie = true;
	}

	auto file_path = fmt::format("Website/{}/{}", lang, path);

	auto response = drogon::HttpResponse::newFileResponse(file_path);
	if (!std::filesystem::exists(file_path))
	{
		auto message = fmt::format("This file don't exist: {} \n", file_path);
		throw std::filesystem::filesystem_error{ message, file_path,
			std::make_error_code(std::errc::no_such_file_or_directory)};
	}

	if (no_cookie)
	{
		drogon::Cookie cookie("lang", "pl");
		cookie.setHttpOnly(false);
		response->addCookie(cookie);
	}
	callback(response);
}

void getStyle(const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback)
{
	std::string_view style_file = "Website/green.css";
	auto& cookies = req->getCookies();
	bool no_cookie = false;

	if (cookies.contains("style"))
	{
		const auto& style = cookies.at("style");
		if (style == "grey")
			style_file = "Website/grey.css";
	}
	else
	{
		no_cookie = true;
	}

	auto response = drogon::HttpResponse::newFileResponse(style_file.data());
	if (!std::filesystem::exists(style_file))
	{
		auto message = fmt::format("This file don't exist: {} \n", style_file);
		throw std::filesystem::filesystem_error{ message, style_file,
			std::make_error_code(std::errc::no_such_file_or_directory) };
	}

	if (no_cookie)
	{
		drogon::Cookie cookie("style", "green");
		cookie.setHttpOnly(false);
		response->addCookie(cookie);
	}
	callback(response);
}

void Server::run()
{
	auto& app = drogon::app();
	app.addListener("127.0.0.1", 80);
	app.setDocumentRoot("Website/");
	app.enableSession(1800);
	app.registerHandler("/", [](const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& callback)
		{
			getTranslation(req, std::move(callback), "index.html");
		});
	app.registerHandler("/getTranslation/{path}", &getTranslation);
	app.registerHandler("/getStyle", &getStyle);

	fmt::print(fmt::fg(fmt::color::lime), "Server started on {}:{} \n", "http://127.0.0.1", 80);
	app.run();
}
