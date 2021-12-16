var main_url = 'http://localhost/';
var map = L.map('map').setView([52.018, 19.137], 6);

var openStreetMap = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map)

var geoJSON = L.geoJSON();
var selection = L.geoJSON();
var speed_map = 0;

function setSpeedMap() {
	if (!speed_map) {
		speed_map = true;
		openStreetMap.setOpacity(0.25);
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
	iconUrl: 'Icons/train.png'
})
var disused_station_icon = new TrainMarker_Large({
	iconUrl: 'Icons/train_old.png'
})
var halt_icon = new TrainMarker_Small({
	iconUrl: 'Icons/train_small.png'
})
var disused_halt_icon = new TrainMarker_Small({
	iconUrl: 'Icons/train_old.png'
})

function popUps(feature, layer) {
	if (feature.geometry.type == 'LineString') {
		var text = "";
		if (feature.properties.line)
			text += "Linia: <strong>" + feature.properties.line + '</strong><br>';
		if (feature.properties.maxspeed)
			text += "Prędkość maksymalna na odcinku: <strong>" + feature.properties.maxspeed + ' </strong>km/h';
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
		text += ': <strong>' + feature.properties.name + '<strong><br>';
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

	var url = main_url + 'bounds/';
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
var selection_to_remove = false;
map.on('mouseout', () => {
	if(selection_to_remove) {
		selection.remove();
		selection_to_remove = false;
	}
}	);

function select(ID, type) {
	if(type == 'rail_station')
		selectStation(ID);
	else if(type == 'rail_line')
		selectLine(ID);
}

function selectStation(ID) {
	url = main_url + 'getElement/station/' + ID;
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			var coords = result.geometry.coordinates;
			var c = L.latLng(coords[1], coords[0]);
			map.setZoom(14);
			map.flyTo(c);
		});
}

function selectLine(ID) {
	url = main_url + 'getElement/rail_line/' + ID;
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			var boundry_polygon = result.properties.boundry;
			var coords = boundry_polygon.coordinates[0];
			var c1 = L.latLng(coords[0][1], coords[0][0]);
			var c2 = L.latLng(coords[2][1], coords[2][0]);
			map.fitBounds(L.latLngBounds(c1, c2));

			selection.remove();
			selection = L.geoJSON(result.properties.features, {
				style: { color: "#000000", weight: 15 }
			});
			selection.addTo(map);
			selection_to_remove = true;
		});
}

const type_radio_buttons = document.querySelectorAll('input[name="searchType"]');
const orginal_rt = document.getElementById('SearchResult_JSON').innerHTML;

var search_limit = 25;
function Search() {
	let type;
	for (const rb of type_radio_buttons) {
		if (rb.checked) {
			type = rb.value;
			break;
		}
	}
	const searchText = document.getElementById('SearchText');
	if(!searchText.value)
		return;
	url = main_url + 'findElement/' + searchText.value + '/' + type;
	if(search_limit != 5)
		url += '/' + search_limit;

	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			let table = document.getElementById('SearchResult');
				table.style.display = "block";
			let html = document.getElementById('SearchResult_JSON');
			let rt = "";
			if(result) {
				for (const obj of result) {
					rt += '<tr class="result_table_row">';
					rt += '<th class="result_table"><button class="result_ID" onclick="select(';
					rt += obj["id"] + ', ' + "'" + type + "'" + ')">\n' + obj["id"] + '</button></th>';
					rt += '<th class="result_table">' + obj["name"] + '</th>';
					rt += '</tr>';
				}
			}
			else {
				rt = "Brak wyników";
			}
			html.innerHTML = orginal_rt + rt;
	});
}

var search_box = document.getElementById("SearchText");
search_box.addEventListener("keyup", function(event) {
	if (event.keyCode === 13) {
		event.preventDefault();
		Search();
	}
});