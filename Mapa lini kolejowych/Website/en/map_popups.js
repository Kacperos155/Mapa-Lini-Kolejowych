function popUps(feature, layer) {
	let text = "";
	let type = "";
	if (feature.geometry.type == 'LineString') {
		type = "way";
		if (feature.properties.number)
			text += "Line: <strong>" + feature.properties.number + '</strong><br>';
		if (feature.properties.max_speed)
			text += "Max speed: <strong>" + feature.properties.max_speed + ' </strong>km/h<br>';
		if (feature.properties.electrified)
			if (feature.properties.electrified == true)
				text += "Line is electrified<br>";
	}
	else if (feature.geometry.type == "MultiLineString") {
		type = "relation";
		if (feature.properties.number)
			text += "Line: <strong>" + feature.properties.number + '</strong><br>';
		if (feature.properties.name)
			text += "Name: <strong>" + feature.properties.name + ' </strong><br>';
	}
	else if (feature.geometry.type == 'Point') {
		type = "node";
		switch (feature.properties.type) {
			case 1: text = 'Station'; break;
			case 2: text = 'Stop'; break;
			case 3: text = 'Disused station'; break;
			case 4: text = 'Disused stop'; break;
		}
		text += ': <strong>' + feature.properties.name + '<strong><br>';
	}

	let id = feature.properties.id;
	if (id && id.at(0) != '_') {
		text += '<br><a href="https://www.openstreetmap.org/' + type + '/' + id
			+ '" target="_blank" rel="noopener noreferrer">';
		text += 'Open in OpenStreetMap'
		text += '</a>';
	}
	else {
		text += "ID: " + feature.properties.id;
    }
	if (text)
		layer.bindPopup(text);
}