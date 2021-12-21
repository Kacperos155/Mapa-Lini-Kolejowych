let main_url = location.origin;
if (map == null) {
	var map = L.map('map');
}

map.setView([52.018, 19.137], 6);
let openStreetMap = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map)

let geoJSON = L.geoJSON();
let selection = L.geoJSON();
let speed_map = 0;

function setSpeedMap() {
	if (!speed_map) {
		speed_map = true;
		openStreetMap.setOpacity(0.25);
	}
	else if (speed_map) {
		speed_map = false;
		openStreetMap.setOpacity(1);
	}
}

function lineStyle(feature) {
	let disusage = feature.properties.disusage;
	let maxspeed = feature.properties.maxspeed;
	let between = (min, x, max) => x >= min && max > x;

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

function addSegments(segments) {
	try {
		let old_geoJSON = geoJSON;
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
		.then(result => addSegments(result));
}

getSegments();
map.on('moveend', getSegments);

let selection_to_remove = false;
map.on('mouseout', () => {
	if (selection_to_remove) {
		selection.remove();
		selection_to_remove = false;
	}
});

function selectStation(ID) {
	url = main_url + '/getElement/rail_station/' + ID;
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			let coords = result.geometry.coordinates;
			let c = L.latLng(coords[1], coords[0]);
			map.setZoom(14);
			map.flyTo(c);
		});
}

function selectLine(ID) {
	url = main_url + '/getElement/rail_line/' + ID;
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			let coords = result.geometry.coordinates[0];
			let c1 = L.latLng(coords[0][1], coords[0][0]);
			let c2 = L.latLng(coords[2][1], coords[2][0]);
			map.fitBounds(L.latLngBounds(c1, c2));

			selection.remove();
			selection = L.geoJSON(result.properties.features, {
				style: { color: "#000000", weight: 15 }
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