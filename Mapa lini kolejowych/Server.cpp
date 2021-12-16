#include "Server.h"

void Server::run()
{
	drogon::app().addListener("0.0.0.0", 80);
	drogon::app().setDocumentRoot("Website/");
	drogon::app().run();
}
