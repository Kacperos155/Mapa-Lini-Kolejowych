#include "utilities.h"
#include <fmt/core.h>

bool utilities::loadSpatiaLite(SQLite::Database& database)
{
	try
	{
		database.loadExtension("mod_spatialite.dll", nullptr);
	}
	catch (const SQLite::Exception& e)
	{
		fmt::print("Loading SpatiaLite failed!\n{}\n", e.what());
		return false;
	}
	catch (...)
	{
		return false;
	}
	return true;
}

std::string utilities::getGeometryBoundry(SQLite::Database& database, std::string_view spatiaLiteGeometry)
{
	auto boundry_statement = fmt::format("SELECT AsText(Envelope(GeomFromText(\'{}\', 4326)));", spatiaLiteGeometry);
	return database.execAndGet(boundry_statement).getText();
}

std::string utilities::asGeoJSON(SQLite::Database& database, std::string_view spatiaLiteGeometry)
{
	auto boundry_statement = fmt::format("SELECT AsGeoJSON(GeomFromText(\"{}\", 4326));", spatiaLiteGeometry);
	return database.execAndGet(boundry_statement).getText();
}
