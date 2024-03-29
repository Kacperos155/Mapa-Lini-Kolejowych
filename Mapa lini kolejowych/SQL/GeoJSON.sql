#pragma once
namespace sql {
namespace GeoJson {

constexpr auto all_lines = R"(
SELECT ID, "Number", "Color", "Name", "Network", "Operator", AsGeoJSON(Line) AS 'GeoJson'
	FROM "Railway lines";
)";

constexpr auto lines_from_tiles = R"(
SELECT ID, "Number", "Color", "Name", "Network", "Operator", AsGeoJSON(Line) AS 'GeoJson'
	FROM "Railway lines"
	WHERE TRUE <= Intersects(Boundry, GeomFromText("{}", 4326));
)";

constexpr auto segments_from_tiles = R"(
SELECT ID, "Line name", "Usage", "Disusage", "Max speed", "Electrified", AsGeoJSON(Line) AS 'GeoJson'
	FROM "Railways"
	WHERE TRUE <= Intersects(Boundry, GeomFromText("{}", 4326));
)";

constexpr auto main_stations_from_tiles = R"(
SELECT "ID", "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Railway stations"
	WHERE "Type" == 1 
		AND 
		TRUE <= Intersects(Point, GeomFromText("{}", 4326));;
)";

constexpr auto stations_from_tiles = R"(
SELECT "ID", "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Railway stations"
	WHERE TRUE <= Intersects(Point, GeomFromText("{}", 4326));
)";

constexpr auto get_segment = R"(
SELECT "ID", "Line name", "Usage", "Disusage", "Max speed", "Electrified", AsGeoJSON(Line) AS 'GeoJson', AsGeoJSON(Boundry) AS 'Boundry'
	FROM "Railway"
	WHERE ID = ?;
)";

constexpr auto get_rail_line = R"(
SELECT "ID", "Number", "Color", "Name", "Network", "Operator", AsGeoJSON(Line) AS 'GeoJson', AsGeoJSON(Boundry) AS 'Boundry'
	FROM "Railway lines"
	WHERE ID = ?;
)";

constexpr auto get_station = R"(
SELECT "ID", "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Railway stations"
	WHERE ID = ?;
)";
}}