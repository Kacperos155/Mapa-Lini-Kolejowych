var map = L.map('map').setView([52.018, 19.137], 6);

var openStreetMap = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

var geoJSON = L.geoJSON();
var speed_map = 1;

function setSpeedMap(bool) {
	if (!speed_map && bool == true) {
		speed_map = true;
		map.remove(openStreetMap);
	}
	else if (speed_map && bool == false) {
		speed_map = false;
		openStreetMap.addTo(map);
    }
}
setSpeedMap(0);

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
		return { color: "#c2c2c2" }
	}
	return { color: "#445da7" }
}

var TrainMarker_Large = L.Icon.extend({
	options: {
		iconSize: [30, 30],
		//popupAnchor: [-3, -76]
	}
});

var TrainMarker_Small = L.Icon.extend({
	options: {
		iconSize: [20, 20],
		//popupAnchor: [-3, -76]
	}
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
			text += "Predkosc maksymalna na odcinku: <b>" + feature.properties.maxspeed + ' </b>km/h';
		if (text)
			layer.bindPopup(text);
	}
	else if (feature.geometry.type == 'Point') {
		var text = "";
		var type = feature.properties.type;
		switch (type) {
			case 1: text = 'Dworzec'; break;
			case 2: text = 'Stacja'; break;
			case 3: text = 'Nieuzywany dworzec'; break;
			case 4: text = 'Nieuzywana stacja'; break;
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
		var kek = 0;
		map.eachLayer(() => ++kek);
		console.log('Segments: ' + kek);
		var kek = 0;
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