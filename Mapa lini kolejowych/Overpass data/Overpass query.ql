auto overpass_query = R"ql(
[out:json][timeout:1800];
area["ISO3166-1:alpha2"="PL"][admin_level=2] -> .country;
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

.tracks out qt tags geom;
.rail_lines out qt body;
.stations out qt body;
)ql";



//Smaller query for tests
auto overpass_query_pomorskie = R"ql(
[out:json][timeout:1800];
area["ISO3166-2"="PL-22"][admin_level=4] -> .country;
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

.tracks out qt tags geom;
.rail_lines out qt body;
.stations out qt body;
)ql";