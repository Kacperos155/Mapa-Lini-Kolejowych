var map = L.map('map').setView([52.018, 19.137], 6);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

var geoJSON = L.geoJSON();

function addSegments(segments) {
	try {
		var kek = 0;
		map.eachLayer(() => ++kek);
		console.log('Segments: ' + kek);
		console.log('Zoom: ' + map.getZoom());
		var kek = 0;
		var old_geoJSON = geoJSON;
		geoJSON = L.geoJSON(segments).addTo(map);
		old_geoJSON.remove();
	}
	catch(error) {
		console.error(error);
	}
}

function getSegments() {
	var bounds = map.getBounds();

	var url = 'http://localhost:2137/bounds/';
	url += bounds.getSouth() + '/';
	url += bounds.getWest() + '/';
	url += bounds.getNorth() + '/';
	url += bounds.getEast();

	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then(result => addSegments(result));
}

getSegments();
map.on('moveend', getSegments);