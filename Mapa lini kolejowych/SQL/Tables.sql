namespace sql {
auto tables = R"(
SELECT InitSpatialMetaData();

DROP TABLE IF EXISTS "Segments";
DROP TABLE IF EXISTS "Rail lines";
DROP TABLE IF EXISTS "Rail lines segments";

CREATE TABLE "Segments" (
	"ID" INTEGER PRIMARY KEY
);
SELECT AddGeometryColumn('Segments', 'Boundry', 4326, 'POLYGON', 2, 1);
SELECT AddGeometryColumn('Segments', 'Line', 4326, 'LINESTRING', 2, 1);

CREATE TABLE "Rail lines" (
	"ID" INTEGER PRIMARY KEY,
	"Name" TEXT NOT NULL
);
SELECT AddGeometryColumn('Rail lines', 'Boundry', 4326, 'POLYGON', 2, 1);

CREATE TABLE "Rail lines segments" (
	"Line_ID" INTEGER,
	"Segment_ID" INTEGER
);
)";

auto segment_insert = R"(
	INSERT INTO "Segments" VALUES (:id, GeomFromText(:boundry, 4326), GeomFromText(:line, 4326));
)";

auto rail_line_insert = R"(
	INSERT INTO "Rail lines" VALUES (:id, :name, GeomFromText(:boundry));
)";

auto rail_line_segment_insert = R"(
	INSERT INTO "Rail lines segments" VALUES (?, ?);
)";
}