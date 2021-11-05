var map = L.map('map').setView([52.018, 19.137], 6);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

//var url = 'http://localhost:2137/bounds/POLYGON((51.11386850819646,16.825561523437504,53.79091696637288,16.825561523437504,53.79091696637288,21.220092773437504,51.11386850819646,21.220092773437504))';
var geoJSON = L.geoJSON();

function addSegments(segments) {
	try {
		var kek = 0;
		map.eachLayer(() => ++kek);
		console.log(kek);
		var kek = 0;
		var old_geoJSON = geoJSON;
		geoJSON = L.geoJSON(segments).addTo(map);
		old_geoJSON.remove();
	}
	catch(error) {
		console.error(error);
	}
}

function downloadSegments(url, func) {
fetch(url)
	.then(response => response.json())
	.then(result => func(result));
}

function getSegments() {
	var bounds = map.getBounds();
	var LatLng_to_String = (LatLng) => LatLng.lng + ' ' + LatLng.lat;

	var url = 'http://localhost:2137/bounds/';
	url += bounds.getSouth() + '/';
	url += bounds.getWest() + '/';
	url += bounds.getNorth() + '/';
	url += bounds.getEast();

	console.log(url);
	downloadSegments(url, addSegments);
}

getSegments();
map.on('moveend', getSegments);