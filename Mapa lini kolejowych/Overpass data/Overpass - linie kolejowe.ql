[out:json][timeout:3600];
area["ISO3166-1:alpha2"="PL"][admin_level=2] -> .country;
(
	way(area.country)["railway"="rail"];
	way(area.country)["railway"="disused"];
    way(area.country)["railway"="preserved"];
) -> .tracks;
(
	way.tracks;
	-
    way(area.country)["usage"="military"];
) -> .tracks;
relation(bw.tracks) -> .rail_lines;
way(r.rail_lines) -> .rail_lines;
way.tracks.rail_lines -> .final;
(
node(w.final);
.final;
relation(bw.final);
) -> ._;
out bb qt;