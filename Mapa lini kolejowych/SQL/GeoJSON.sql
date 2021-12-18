#pragma once
namespace sql {
namespace GeoJson {

auto all_lines = R"(
SELECT ID, "Number", "Name", "Network", "Operator", AsGeoJSON(Line) AS 'GeoJson'
	FROM "Rail lines";
)";

auto lines_in_bound = R"(
SELECT ID, "Number", "Name", "Network", "Operator", AsGeoJSON(Line) AS 'GeoJson'
	FROM "Rail lines"
	WHERE TRUE <= Intersects(Boundry, GeomFromText(?, 4326));
)";

auto segments_in_bound = R"(
SELECT ID, "Line number", "Usage", "Disusage", "Max speed", "Voltage", AsGeoJSON(Line) AS 'GeoJson'
	FROM Segments
	WHERE TRUE <= Intersects(Boundry, GeomFromText(?, 4326));
)";

auto main_stations_in_bound = R"(
SELECT "ID", "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM 
	(
		SELECT *
		FROM "Rail stations"
		WHERE "Type" == 1
	)
	WHERE TRUE <= Intersects(Point, GeomFromText(?, 4326));
)";

auto stations_in_bound = R"(
SELECT "ID", "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Rail stations"
	WHERE TRUE <= Intersects(Point, GeomFromText(?, 4326));
)";

auto get_station_location = R"(
SELECT "ID", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Rail stations"
	WHERE ID = ?;
)";

auto get_rail_line_location = R"(
SELECT "ID", AsGeoJSON(Boundry) AS 'GeoJson'
	FROM "Rail lines"
	WHERE ID = ?;
)";
}}