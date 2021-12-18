#pragma once
namespace sql {
namespace query {

auto get_rail_line_line_strings = R"(
SELECT AsText(s.Line) AS 'LineString'
	FROM (
		SELECT Segment_ID
		FROM "Rail lines segments"
		WHERE Line_ID = ?
	) AS 'w'
	INNER JOIN "Segments" AS 's' 
	ON w.Segment_ID = s.ID;
)";

auto get_rail_line = R"(
SELECT "ID", "Number", "Name", "Network", "Operator"
	FROM "Rail lines"
	WHERE ID = ?;
)";

auto get_segment = R"(
SELECT "ID", "Line number", "Usage" "Disusage", "Max speed", "Voltage"
	FROM "Segments"
	WHERE ID = ?;
)";

auto get_station = R"(
SELECT "ID", "Name", "Type"
	FROM "Rail stations"
	WHERE ID = ?;
)";

auto search_station = R"(
SELECT "ID", "Name"
	FROM "Rail stations"
	WHERE "Name" LIKE ?
	LIMIT ?;
)";

auto search_rail_line = R"(
SELECT "ID", "Name"
	FROM "Rail lines"
	WHERE "Name" LIKE ?
	LIMIT ?;
)";
}}