#pragma once
#include <string>
#include <string_view>

#include "Railnode.hpp"
#include "Railway.hpp"
#include "Railway_line.hpp"
#include "Railway_station.hpp"

struct Variable
{
	std::string key;
	std::string value;

	static constexpr std::string_view sql_table_name = "Variables";
	static constexpr std::string_view sql_create =
		R"(
DROP TABLE IF EXISTS "Variables";
CREATE TABLE "Variables" (
	"Key" TEXT PRIMARY KEY,
	"Value" TEXT
);
)";

	static constexpr std::string_view sql_insert =
		R"(
	INSERT INTO "Variables" ("Key", "Value")
	VALUES (?, ?);
)";

	static constexpr std::string_view sql_get =
		R"(
SELECT "Value"
	FROM "Variables"
	WHERE Key = ?;
)";
};

static constexpr std::string_view sql_init_spatialite = "SELECT InitSpatialMetaData();";