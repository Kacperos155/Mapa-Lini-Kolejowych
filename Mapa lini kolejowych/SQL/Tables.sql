#pragma once
namespace sql {
auto tables = R"(
SELECT InitSpatialMetaData();

DROP TABLE IF EXISTS "Info";
DROP TABLE IF EXISTS "Segments";
DROP TABLE IF EXISTS "Rail lines";
DROP TABLE IF EXISTS "Rail lines segments";
DROP TABLE IF EXISTS "Rail stations";

CREATE TABLE "Info" (
	"Key" TEXT PRIMARY KEY,
	"Value" TEXT
);

CREATE TABLE "Segments" (
	"ID" TEXT PRIMARY KEY,
	"Line number" TEXT NOT NULL,
	"Usage" TEXT,
	"Disusage" INTEGER,
	"Max speed" INTEGER,
	"Voltage" INTEGER
);
SELECT AddGeometryColumn('Segments', 'Boundry', 4326, 'POLYGON', 2, 1);
SELECT AddGeometryColumn('Segments', 'Line', 4326, 'LINESTRING', 2, 1);

CREATE TABLE "Rail lines" (
	"ID" TEXT PRIMARY KEY,
	"Number" TEXT NOT NULL,
	"Color" TEXT NOT NULL,
	"Name" TEXT,
	"Network" TEXT,
	"Operator" TEXT
);
SELECT AddGeometryColumn('Rail lines', 'Boundry', 4326, 'POLYGON', 2, 1);
SELECT AddGeometryColumn('Rail lines', 'Line', 4326, 'MULTILINESTRING', 2, 1);

CREATE TABLE "Rail lines segments" (
	"Line_ID" TEXT,
	"Segment_ID" TEXT
);

CREATE TABLE "Rail stations" (
	"ID" TEXT PRIMARY KEY,
	"Name" TEXT NOT NULL,
	"Type" INT NOT NULL
);
SELECT AddGeometryColumn('Rail stations', 'Point', 4326, 'POINT', 2, 1);
)";

auto info_insert = R"(
	INSERT INTO "Info" ("Key", "Value")
	VALUES (?, ?);
)";

auto segment_insert = R"(
	INSERT INTO "Segments" ("ID", "Line number", "Usage", "Disusage", "Max speed", "Voltage", "Boundry", "Line")
	VALUES (:id, :line_number, :usage, :disusage, :max_speed, :voltage, GeomFromText(:boundry, 4326), GeomFromText(:line, 4326));
)";

auto rail_line_insert = R"(
	INSERT INTO "Rail lines" ("ID", "Number", "Color", "Name", "Network", "Operator", "Boundry", "Line")
	VALUES (:id, :number, :color, :name, :network, :operator, GeomFromText(:boundry, 4326), GeomFromText(:line, 4326));
)";

auto rail_line_segment_insert = R"(
	INSERT INTO "Rail lines segments" VALUES (?, ?);
)";

auto rail_station_insert = R"(
	INSERT INTO "Rail stations" ("ID", "Name", "Type", "Point")
	VALUES (:id, :name, :type, GeomFromText(:point, 4326));
)";
}