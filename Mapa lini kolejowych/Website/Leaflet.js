var map = L.map('map').setView([52.018, 19.137], 6);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

var marker = L.marker([52.018, 19.137]).addTo(map);

function getGeometry(e){
    var center = map.getCenter();
    marker.setLatLng(center);
}

map.on('moveend', getGeometry);
map.on('zoomend', getGeometry);
map.on('viewreset', getGeometry);
//map.on('click', onMapClick);