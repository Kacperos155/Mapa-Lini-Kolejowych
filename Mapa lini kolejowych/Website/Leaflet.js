let main_url = location.origin;
if (map == null) {
	var map = L.map('map');
}

console.log(map);

map.setView([52.018, 19.137], 6);
let openStreetMap = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map)

L.control.scale({ imperial: false }).addTo(map);
map.attributionControl.addAttribution("Kacper Zielinski 2022");

let geoJSON = {};
let geoJSONLayer = L.geoJSON();
let selection = L.geoJSON();
let speed_map = 0;

function setSpeedMap() {
	const button = document.querySelector('button[onclick="setSpeedMap()"]');

	if (!speed_map) {
		speed_map = true;
		openStreetMap.setOpacity(0.25);
		button.className = "toggled";
	}
	else if (speed_map) {
		speed_map = false;
		openStreetMap.setOpacity(1);
		button.className = "untoggled";
	}
	refreshGeoJson(geoJSON);
	refreshLegend(speed_map);
}

const speedLimits = [
	{
		min_speed: 1,
		color: "rgb(255,0,0)"
	},
	{
		min_speed: 40,
		color: "rgb(255,100,0)"
	},
	{
		min_speed: 60,
		color: "rgb(255,180,0)"
	},
	{
		min_speed: 80,
		color: "rgb(255,255,0)"
	},
	{
		min_speed: 100,
		color: "rgb(120,150,0)"
	},
	{
		min_speed: 120,
		color: "rgb(0,255,0)"
	},
	{
		min_speed: 140,
		color: "rgb(0,255,255)"
	}
]

const defaultLineColor = "rgb(50, 120, 230)";

function speedLimitColor(speedLimit) {
	let between = (min, x, max) => min <= x && x < max;
	let currentLimit = {};

	if (0 < speedLimit) {
		for (const limit of speedLimits) {
			if (limit.min_speed > speedLimit)
				return currentLimit;
			currentLimit = limit;
        }
	}
	return { color: "rgb(255,255,255)" };
}

function lineStyle(feature) {
	let number = feature.properties.number;

	return { color: defaultLineColor };
}

function segmentStyle(feature) {
	let disusage = feature.properties.disusage;

	if (speed_map) {
		return { color: speedLimitColor(feature.properties.max_speed).color };
	}
	if (disusage)
		return { interactive: false, color: "rgb(0,0,0)", "opacity": 0.8 };
	return { color: defaultLineColor }
}

let TrainMarker_Large = L.Icon.extend({
	options: { iconSize: [30, 30] }
});

let TrainMarker_Small = L.Icon.extend({
	options: { iconSize: [20, 20] }
});

//Icons from freepik.com
let station_icon = new TrainMarker_Large({
	iconUrl: '/Icons/train.png'
})
let disused_station_icon = new TrainMarker_Large({
	iconUrl: '/Icons/train_old.png'
})
let halt_icon = new TrainMarker_Small({
	iconUrl: '/Icons/train_small.png'
})
let disused_halt_icon = new TrainMarker_Small({
	iconUrl: '/Icons/train_old.png'
})

function pointStyle(feature, LatLng) {
	let type = feature.properties.type;

	switch (feature.properties.type) {
		case 1: return L.marker(LatLng, { icon: station_icon });
		case 2: return L.marker(LatLng, { icon: halt_icon });
		case 3: return L.marker(LatLng, { icon: disused_station_icon });
		case 4: return L.marker(LatLng, { icon: disused_halt_icon });
		default: return {};
	}
}

let geojsonMarkerOptions = {
	radius: 8,
	fillColor: "#ff7800",
	color: "#000",
	weight: 1,
	opacity: 1,
	fillOpacity: 0.8
};

function refreshGeoJson(features) {
	let old_geoJSON = geoJSONLayer;
	try {
		let newGeoJSONLayer = L.geoJSON(features, {
			pointToLayer: function (feature, latlng) {
				return pointStyle(feature, latlng);
			},
			onEachFeature: popUps,
			filter: (feature) => {
				if (speed_map && feature.geometry.type == 'MultiLineString' && map.getZoom() >= 10) {
					return false;
				}
				else if (!speed_map && feature.geometry.type == 'LineString') {
					if (feature.properties.disusage)
						return true;
					return false;
				}
				else
					return true;
			},
			style: (feature) => {
				if (feature.geometry.type == 'LineString') {
					return segmentStyle(feature);
				}
				else if (feature.geometry.type == 'MultiLineString')
					return lineStyle(feature);
			}
		});
		old_geoJSON.remove();
		geoJSONLayer = newGeoJSONLayer;
		geoJSONLayer.addTo(map);
		geoJSON = features;
	}
	catch (error) {
		console.error(error);
		geoJSONLayer = old_geoJSON;
	}
}

function getGeoJson() {
	let bounds = map.getBounds();

	let url = main_url + '/bounds/';
	url += bounds.getSouth() + '/';
	url += bounds.getWest() + '/';
	url += bounds.getNorth() + '/';
	url += bounds.getEast() + '/';
	url += map.getZoom();

	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then(result => refreshGeoJson(result));
}

getGeoJson();
map.on('moveend', getGeoJson);

let selection_to_remove = false;
map.on('mouseout', () => {
	if (selection_to_remove) {
		selection.remove();
		selection_to_remove = false;
	}
});

function selectStation(ID) {
	url = main_url + '/getElement/rail_station/' + ID;
	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			let coords = result.geometry.coordinates;
			let c = L.latLng(coords[1], coords[0]);
			map.flyTo(c, 14, { duration: 1.5, noMoveStart: true });
		});
}

function selectLine(ID) {
	url = main_url + '/getElement/rail_line/' + ID;
	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			if (result.properties.bounds) {
				let coords = result.properties.bounds.coordinates[0];
				let c1 = L.latLng(coords[0][1], coords[0][0]);
				let c2 = L.latLng(coords[2][1], coords[2][0]);
				map.fitBounds(L.latLngBounds(c1, c2));
			}

			selection.remove();
			selection = L.geoJSON(result, {
				style: { color: "#ffffff", weight: 15 }
			});
			selection.addTo(map);
			selection_to_remove = true;
		});
}

window.addEventListener('message', (event) => {
	if (event.origin == location.origin) {
		let delimiter = event.data.indexOf(';');
		let message_type = "";
		if (delimiter != -1) {
			message_type = event.data.substring(0, delimiter);
		}
		if (message_type == "select") {
			let start = delimiter + 1;
			delimiter = event.data.indexOf(';', start);
			let type = event.data.substring(start, delimiter);
			let id = event.data.substring(delimiter + 1);

			if (type == 'rail_station')
				selectStation(id);
			else if (type == 'rail_line')
				selectLine(id);
		}
	}
})