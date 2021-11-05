var map = L.map('map').setView([52.018, 19.137], 6);

var openStreetMap = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map)

var geoJSON = L.geoJSON();
var speed_map = 0;

function setSpeedMap() {
	if (!speed_map) {
		speed_map = true;
		openStreetMap.setOpacity(0.05);
	}
	else if (speed_map) {
		speed_map = false;
		openStreetMap.setOpacity(1);
	}
	geoJSON.setStyle((feature) => {
		if (feature.geometry.type == 'LineString')
			return lineStyle(feature)
	});
}

function lineStyle(feature) {
	var disusage = feature.properties.disusage;
	var maxspeed = feature.properties.maxspeed;
	var between = (min, x, max) => x >= min && max > x;

	if (disusage)
		return { color: "#4a4a4a", "opacity": 0.8 };
	if (speed_map) {
		if (between(1, maxspeed, 40))
			return { color: "#ff0000" };
		if (between(40, maxspeed, 70))
			return { color: "#ff9500" };
		if (between(70, maxspeed, 100))
			return { color: "#f7de3b" };
		if (between(100, maxspeed, 120))
			return { color: "#d5d73c" };
		if (between(120, maxspeed, 140))
			return { color: "#bdf94d" };
		if (between(140, maxspeed, 300))
			return { color: "#21f24b" };
		return { color: "#ffffff" }
	}
	return { color: "#445da7" }
}

var TrainMarker_Large = L.Icon.extend({
	options: { iconSize: [30, 30] }
});

var TrainMarker_Small = L.Icon.extend({
	options: { iconSize: [20, 20] }
});

//Icons from freepik.com
var station_icon = new TrainMarker_Large({
	iconUrl: 'train.png'
})
var disused_station_icon = new TrainMarker_Large({
	iconUrl: 'train_old.png'
})
var halt_icon = new TrainMarker_Small({
	iconUrl: 'train_small.png'
})
var disused_halt_icon = new TrainMarker_Small({
	iconUrl: 'train_old.png'
})

function popUps(feature, layer) {
	if (feature.geometry.type == 'LineString') {
		var text = "";
		if (feature.properties.line)
			text += "Linia: <b>" + feature.properties.line + '</b><br/>';
		if (feature.properties.maxspeed)
			text += "Prędkość maksymalna na odcinku: <b>" + feature.properties.maxspeed + ' </b>km/h';
		if (text)
			layer.bindPopup(text);
	}
	else if (feature.geometry.type == 'Point') {
		var text = "";
		var type = feature.properties.type;
		switch (type) {
			case 1: text = 'Dworzec'; break;
			case 2: text = 'Stacja'; break;
			case 3: text = 'Nieużywany dworzec'; break;
			case 4: text = 'Nieużywana stacja'; break;
		}
		text += ': <b>' + feature.properties.name + '</b><br/>';
		layer.bindPopup(text);
	}
	if (feature.properties && feature.properties.popupContent) {
		layer.bindPopup(feature.properties.popupContent);
	}
}

function pointStyle(feature, LatLng) {
	var type = feature.properties.type;

	switch (feature.properties.type) {
		case 1: return L.marker(LatLng, { icon: station_icon });
		case 2: return L.marker(LatLng, { icon: halt_icon });
		case 3: return L.marker(LatLng, { icon: disused_station_icon });
		case 4: return L.marker(LatLng, { icon: disused_halt_icon });
		default: return {};
	}
}

var geojsonMarkerOptions = {
	radius: 8,
	fillColor: "#ff7800",
	color: "#000",
	weight: 1,
	opacity: 1,
	fillOpacity: 0.8
};

function addSegments(segments) {
	try {
		var old_geoJSON = geoJSON;
		geoJSON = L.geoJSON(segments, {
			pointToLayer: function (feature, latlng) {
				return pointStyle(feature, latlng);
			},
			onEachFeature: popUps,
			style: (feature) => {
				if (feature.geometry.type == 'LineString')
					return lineStyle(feature);
			}
		}).addTo(map);
		old_geoJSON.remove();
	}
	catch (error) {
		console.error(error);
	}
}

function getSegments() {
	var bounds = map.getBounds();

	var url = 'http://localhost:2137/bounds/';
	url += bounds.getSouth() + '/';
	url += bounds.getWest() + '/';
	url += bounds.getNorth() + '/';
	url += bounds.getEast() + '/';
	url += map.getZoom();

	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then(result => addSegments(result));
}

getSegments();
map.on('moveend', getSegments);

function selectLine(ID) {
	url = 'http://localhost:2137/get/rail_line/' + ID;
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			var boundry_polygon = result.properties.boundry;
			var coords = boundry_polygon.coordinates[0];
			var c1 = L.latLng(coords[0][1], coords[0][0]);
			var c2 = L.latLng(coords[2][1], coords[2][0]);
			map.fitBounds(L.latLngBounds(c1, c2));

			//L.geoJSON(boundry_polygon).addTo(map);
			L.geoJSON(result.properties.features, {
				style: { color: "#000000", weight: 8 }
			}).addTo(map);
		});
}
//selectLine(1711959);