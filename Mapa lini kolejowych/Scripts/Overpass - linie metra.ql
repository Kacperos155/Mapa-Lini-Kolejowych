[out:json][timeout:3600];
area["ISO3166-1:alpha2"="PL"][admin_level=2]->.country;
(
	relation(area.country)["route"="subway"];
);
(._;>;);
out body qt bb;