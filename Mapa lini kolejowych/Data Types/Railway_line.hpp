#pragma once
#include <string>
#include <string_view>
#include <array>

struct Railway_line
{
	uint64_t ID;
	std::string number;
	std::string name;
	std::string network;
	std::string line_operator;

	std::array<char, 6> color;

	std::string from;
	std::string via;
	std::string to;

	std::string boundry;
	std::string line;

	static constexpr std::string_view sql_table_name = "Railway lines";
	static constexpr std::string_view sql_create =
		R"(
DROP TABLE IF EXISTS "Railway stations";
CREATE TABLE "Railway lines" (
	"ID" INT PRIMARY KEY,
	"Number" TEXT NOT NULL,
	"Color" TEXT NOT NULL,
	"Name" TEXT,
	"Network" TEXT,
	"Operator" TEXT,
	"From" TEXT,
	"Via" TEXT,
	"To" TEXT
);
SELECT AddGeometryColumn('Railway lines', 'Boundry', 4326, 'POLYGON', 2, 0); 
SELECT AddGeometryColumn('Railway lines', 'Line', 4326, 'MULTILINESTRING', 2, 0);
-- change last numbers from 0 to 1
)";

	static constexpr std::string_view sql_insert =
		R"(
INSERT INTO "Railway lines" ("ID", "Number", "Color", "Name", "Network", "Operator", "From", "Via", "To", "Boundry", "Line")
	VALUES (:id, :number, :color, :name, :network, :operator, :from, :via, :to, GeomFromText(:boundry, 4326), GeomFromText(:line, 4326));
)";

	static constexpr std::string_view sql_get =
		R"(
SELECT "Number", "Color", "Name", "Network", "Operator"
	FROM "Railway lines"
	WHERE ID = ?;
)";

	static constexpr std::string_view sql_search =
		R"(
SELECT "ID", "Name"
	FROM "Railway lines"
	WHERE "Name" LIKE ?
	LIMIT ?;
)";
};
