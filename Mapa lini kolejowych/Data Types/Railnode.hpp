#pragma once
#include <vector>
#include <string_view>

struct Railnode
{
	int64_t ID : 58 {};
	int64_t ref_counter : 6 {};
	float lon{};
	float lat{};

	std::vector<Railnode*> neighbours;

	Railnode()
	{
		neighbours.reserve(6);
	}

	static constexpr std::string_view sql_table_name = "Railnodes";
	static constexpr std::string_view sql_create_fmt =
		R"(
DROP TABLE IF EXISTS "Railnodes";
CREATE TABLE "Railnodes" (
	"ID" TEXT PRIMARY KEY,
	{}
);
SELECT AddGeometryColumn('Railnodes', 'Point', 4326, 'POINT', 2, 1);
)";

	static constexpr std::string_view sql_insert_fmt =
		R"(
INSERT INTO "Railnodes" ("ID", {}, "Point")
	VALUES (:id, {}, MakePoint(:lon, :lat, 4326));
)";

	static constexpr std::string_view sql_get_all_fmt =
		R"(
SELECT "ID", {}, X(Point) as Lon, Y(Point) as Lat
	FROM "Railnodes"
)";
};