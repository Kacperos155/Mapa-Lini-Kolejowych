namespace sql {
namespace query {
auto point_from_node = R"(
SELECT "Point"
	FROM "Nodes"
	WHERE "ID"=?;
)";

auto segments_in_bound = R"(
SELECT ID, "Usage", "Disusage", "Line number", "Max speed", "Electrified", AsGeoJSON(Line) AS 'GeoJson'
	FROM Segments
	WHERE TRUE <= Intersects(Boundry, GeomFromText(?, 4326));
)";

auto stations_in_bound = R"(
SELECT "ID", "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Rail stations"
	WHERE TRUE <= Intersects(Point, GeomFromText(?, 4326));
)";

auto get_station = R"(
SELECT "Name", "Type", AsGeoJSON(Point) AS 'GeoJson'
	FROM "Rail stations"
	WHERE ID = ?;
)";

auto get_rail_line = R"(
SELECT "Number", "Name", "Network", "Operator", AsGeoJSON(Boundry) AS 'GeoJson'
	FROM "Rail lines"
	WHERE ID = ?;
)";

auto get_rail_line_segments = R"(
SELECT AsGeoJSON(s.Line) AS 'GeoJson'
	FROM (
	SELECT Segment_ID
	FROM "Rail lines segments"
	WHERE Line_ID = ?
	) AS 'w'
	INNER JOIN "Segments" AS 's' ON
		w.Segment_ID = s.ID;
)";

auto get_rail_line_line_strings = R"(
SELECT AsText(s.Line) AS 'LineString'
	FROM (
	SELECT Segment_ID
	FROM "Rail lines segments"
	WHERE Line_ID = ?
	) AS 'w'
	INNER JOIN "Segments" AS 's' ON
		w.Segment_ID = s.ID;
)";

auto search_station = R"(
SELECT "ID", "Name"
	FROM "Rail stations"
	WHERE "Name" LIKE ?
	LIMIT ?;
)";

auto search_rail_lines = R"(
SELECT "ID", "Name"
	FROM "Rail lines"
	WHERE "Name" LIKE ?
	LIMIT ?;
)";
}}