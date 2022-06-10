namespace overpass_query{
// Overpass query to download railway data
// You must replace {} with area definition before use!!!
constexpr auto query = R"ql(
[out:json][timeout:1800];
{} -> .country;
(
	node(area.country)["railway"="station"];
    node(area.country)["railway"="halt"];
	node(area.country)["disused:railway"="station"];
    node(area.country)["disused:railway"="halt"];
) -> .stations;
(
	relation(area.country)["route"="railway"];
	relation(area.country)["route"="tracks"];
) -> .rail_lines;
(
	way(area.country)["railway"="rail"];
	way(area.country)["railway"="disused"];
    way(area.country)["railway"="preserved"];
) -> .tracks;

way(r.rail_lines).tracks -> .tracks;
relation(bw.tracks) -> .rail_lines;

.tracks out qt body geom;
.rail_lines out qt body;
.stations out qt body;
)ql";

// Area of whole country
// Replace {} with country tag
constexpr auto area_country = R"ql(
area["ISO3166-1:alpha2"="{}"][admin_level=2]
)ql";

//Small area for tests
constexpr auto area_pomorskie = R"ql(
area["ISO3166-2"="PL-22"][admin_level=4]
)ql";
}
