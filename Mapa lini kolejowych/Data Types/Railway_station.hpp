#pragma once
#include <string>
#include <string_view>

struct Railnode;

struct Railway_station
{
	int64_t ID{};
	std::string name{};
	Railnode *node{};
	float lon{};
	float lat{};

	enum class Type : uint8_t
	{
		Invalid = 0,
		Station = 1,
		Stop = 2,
		Disused_Station = 3,
		Disused_Stop = 4
	};
	Type type {Type::Invalid };

	std::string point;

	static constexpr std::string_view sql_table_name = "Railway stations";
	static constexpr std::string_view sql_create =
		R"(
DROP TABLE IF EXISTS "Railway stations";
CREATE TABLE "Railway stations" (
	"ID" TEXT PRIMARY KEY,
	"Railnode ID" TEXT NOT NULL,
	"Name" TEXT NOT NULL,
	"Type" INT NOT NULL
);
SELECT AddGeometryColumn('Railway stations', 'Point', 4326, 'POINT', 2, 1);
)";

	static constexpr std::string_view sql_insert =
		R"(
INSERT INTO "Railway stations" ("ID", "Railnode ID", "Name", "Type", "Point")
	VALUES (:id, :node, :name, :type, MakePoint(:lon, :lat, 4326));
)";

	static constexpr std::string_view sql_get_all =
		R"(
SELECT "ID", "Railnode ID", "Name", "Type", X(Point) as Lon, Y(Point) as Lat
	FROM "Railway stations"
)";

	static constexpr std::string_view sql_get =
		R"(
SELECT "Name", "Type"
	FROM "Railway stations"
	WHERE ID = ?;
)";

	static constexpr std::string_view sql_search =
		R"(
SELECT "ID", "Name"
	FROM "Railway stations"
	WHERE "Name" LIKE ?
	LIMIT ?;
)";
};