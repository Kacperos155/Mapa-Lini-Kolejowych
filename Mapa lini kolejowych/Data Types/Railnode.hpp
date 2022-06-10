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
		neighbours.reserve(4);
	}

	static constexpr std::string_view sql_table_name = "Railnodes";
	static constexpr std::string_view sql_create =
		R"(
DROP TABLE IF EXISTS "Railnodes";
CREATE TABLE "Railnodes" (
	"ID" INT PRIMARY KEY,
	"Ref Counter" INT NOT NULL
);
SELECT AddGeometryColumn('Railnodes', 'Point', 4326, 'POINT', 2, 1);
)";

	static constexpr std::string_view sql_insert =
		R"(
INSERT INTO "Railnodes" ("ID", "Ref Counter", "Point")
	VALUES (:id, :ref_counter, GeomFromText(:point, 4326));
)";

	static constexpr std::string_view sql_get =
		R"(
SELECT "Ref Counter", "Point"
	FROM "Railnodes"
	WHERE ID = ?;
)";
};