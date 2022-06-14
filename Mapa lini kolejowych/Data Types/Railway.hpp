#pragma once
#include <string>
#include <string_view>

struct Railway
{
	int64_t ID{};
	std::string line_name{};

	enum class Usage
	{
		Unknown = 0,
		Main = 1,
		Branch,
		Tourism,
		Industrial,
		Freight,
		Military,
	};
	Usage usage{ Usage::Unknown };

	uint16_t max_speed{};
	bool disusage{ false };
	bool electrified{ false };

	std::string boundry{};
	std::string line{};

	static constexpr std::string_view sql_table_name = "Railways";
	static constexpr std::string_view sql_create =
		R"(
DROP TABLE IF EXISTS "Railways";
CREATE TABLE "Railways" (
	"ID" TEXT PRIMARY KEY,
	"Line name" TEXT,
	"Usage" TEXT,
	"Max speed" INTEGER,
	"Disusage" INTEGER,
	"Electrified" INTEGER
);
SELECT AddGeometryColumn('Railways', 'Boundry', 4326, 'POLYGON', 2, 1);
SELECT AddGeometryColumn('Railways', 'Line', 4326, 'LINESTRING', 2, 1);
)";

	static constexpr std::string_view sql_insert =
		R"(
INSERT INTO "Railways" ("ID", "Line name", "Usage", "Max speed", "Disusage", "Electrified", "Boundry", "Line")
	VALUES (:id, :line_name, :usage, :max_speed, :disusage, :electrified, GeomFromText(:boundry, 4326), GeomFromText(:line, 4326));
)";

	static constexpr std::string_view sql_get =
		R"(
SELECT "Line number", "Usage", "Max speed", "Disusage", "Electrified"
	FROM "Railways"
	WHERE ID = ?;
)";
};
