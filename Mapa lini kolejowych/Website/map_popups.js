function popUps(feature, layer) {
	let text = "";
	let type = "";
	if (feature.geometry.type == 'LineString') {
		type = "way";
		if (feature.properties.line)
			text += "!_rail line_!: <strong>" + feature.properties.line + '</strong><br>';
		if (feature.properties.maxspeed)
			text += "!_max speed_!: <strong>" + feature.properties.maxspeed + ' </strong>km/h';
	}
	else if (feature.geometry.type == "MultiLineString") {
		type = "relation";
		if (feature.properties.number)
			text += "!_rail line_!: <strong>" + feature.properties.number + '</strong><br>';
		if (feature.properties.name)
			text += "!_name_!: <strong>" + feature.properties.name + ' </strong><br>';
	}
	else if (feature.geometry.type == 'Point') {
		type = "node";
		switch (feature.properties.type) {
			case 1: text = '!_railway station_!'; break;
			case 2: text = '!_railway stop_!'; break;
			case 3: text = '!_disused railway station_!'; break;
			case 4: text = '!_disused railway stop_!'; break;
		}
		text += ': <strong>' + feature.properties.name + '<strong><br>';
	}

	let id = feature.properties.id;
	if (id && id.at(0) != '_') {
		text += '<br><a href="https://www.openstreetmap.org/' + type + '/' + id
			+ '" target="_blank" rel="noopener noreferrer">';
		text += '!_Open in OpenStreetMap_!'
		text += '</a>';
	}
	if (text)
		layer.bindPopup(text);
}