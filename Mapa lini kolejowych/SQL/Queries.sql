namespace sql {
namespace query {
auto ways_coords = R"(
SELECT n.Latitude, n.Longtitude 
	FROM "Ways" AS 'w'
	INNER JOIN "Ways - Nodes" AS 'wn' ON w.ID = wn.Way_ID
	INNER JOIN "Nodes" AS 'n' ON n.ID = wn.Node_ID; 
)";

auto segment_show = R"(
	select ID, asWKT(Boundry), asWKT(Line) from Segments;
)";
}}