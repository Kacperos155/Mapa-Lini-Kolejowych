let main_url = location.origin;
if (map == null) {
	var map = L.map('map');
}

map.setView([52.018, 19.137], 6);
let openStreetMap = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map)

L.control.scale({ imperial: false }).addTo(map);
map.attributionControl.addAttribution("Kacper Zielinski 2022");

let geoJSON = {};
let geoJSONLayer = L.geoJSON();
let selection = L.geoJSON();
let lines_coloring = false;
let speed_map = false;

function toggleSpeedMap() {
	speed_map = !speed_map;
	const button = document.querySelector('button[onclick="toggleSpeedMap()"]');

	if (speed_map) {
		openStreetMap.setOpacity(0.25);
		button.className = "toggled";
	}
	else {
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

function toggleLinesColoring() {
	lines_coloring = !lines_coloring;
	const button = document.querySelector('button[onclick="toggleLinesColoring()"]');

	if (lines_coloring) {
		button.className = "toggled";
	}
	else {
		button.className = "untoggled";
	}
	refreshGeoJson(geoJSON);
}

function lineStyle(feature) {
	if (lines_coloring && feature.properties.color)
		return { color: '#' + feature.properties.color };

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

function pointStyle(feature, LatLng) {
	let type = feature.properties.type;
	let src = "";
	switch (feature.properties.type) {
		case 1: src = '/Icons/train.png'; break;
		case 2: src = '/Icons/train_small.png'; break;
		case 3: src = '/Icons/train_old.png'; break;
		case 4: src = '/Icons/train_old.png'; break;
	}
	let size = 32;
	if (feature.properties.type == 2 || feature.properties.type == 4)
		size = 20;

	let html = '<img src="' + src + '" width=' + size + ' height=' + size + '>' +
		"<strong>" + feature.properties.name + "</strong>";

	let icon = L.divIcon({ className: "leaflet-divIcon", html: html });

	return L.marker(LatLng, { icon: icon });
}

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
map.on("zoomend", () => { refreshLegend(speed_map); });

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
			console.log(result);
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
		let start = delimiter + 1;
		if (message_type == "select") {
			delimiter = event.data.indexOf(';', start);
			let type = event.data.substring(start, delimiter);
			let id = event.data.substring(delimiter + 1);

			if (type == 'rail_station')
				selectStation(id);
			else if (type == 'rail_line')
				selectLine(id);
		}
		if (message_type == "line") {
			//let result = JSON.parse(event.data.substring(delimiter + 1));
			//uncomment above
			url = main_url + '/getElement/rail_line/319222';
			console.log(url);
			fetch(url)
				.then(response => response.json())
				.then((result) => {
					console.log(result);
					// comment till here
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
				}
			) //remember about this
		}
	}
})
