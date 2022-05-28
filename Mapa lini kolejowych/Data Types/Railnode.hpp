#pragma once
#include <cstdint>

struct Railnode
{
	uint64_t ID : 58;
	uint8_t ref_counter;
	float lon;
	float lat;

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