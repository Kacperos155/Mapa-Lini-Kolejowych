[out:json][timeout:3600];
area["ISO3166-1:alpha2"="PL"][admin_level=2] -> .country;
(
	node(area.country)["railway"="station"];
    node(area.country)["railway"="halt"];
	node(area.country)["disused:railway"="station"];
    node(area.country)["disused:railway"="halt"];
);
out body qt;