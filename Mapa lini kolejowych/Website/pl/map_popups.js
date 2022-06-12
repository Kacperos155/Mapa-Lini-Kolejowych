function popUps(feature, layer) {
	let text = "";
	let type = "";
	if (feature.geometry.type == 'LineString') {
		type = "way";
		if (feature.properties.number)
			text += "Linia: <strong>" + feature.properties.number + '</strong><br>';
		if (feature.properties.max_speed)
			text += "Prędkość maksymalna: <strong>" + feature.properties.max_speed + ' </strong>km/h<br>';
		if (feature.properties.electrified)
			if (feature.properties.electrified == true)
				text += "Linia zelektryfikowana<br>";
	}
	else if (feature.geometry.type == "MultiLineString") {
		type = "relation";
		if (feature.properties.number)
			text += "Linia: <strong>" + feature.properties.number + '</strong><br>';
		if (feature.properties.name)
			text += "Nazwa: <strong>" + feature.properties.name + ' </strong><br>';
	}
	else if (feature.geometry.type == 'Point') {
		type = "node";
		switch (feature.properties.type) {
			case 1: text = 'Dworzec'; break;
			case 2: text = 'Stacja'; break;
			case 3: text = 'Nieużywany dworzec'; break;
			case 4: text = 'Nieużywana stacja'; break;
		}
		text += ': <strong>' + feature.properties.name + '<strong><br>';
	}

	let id = feature.properties.id;
	if (id && id.at(0) != '_') {
		text += '<br><a href="https://www.openstreetmap.org/' + type + '/' + id
			+ '" target="_blank" rel="noopener noreferrer">';
		text += 'Otwórz w OpenStreetMap'
		text += '</a>';
	}
	else {
		text += "ID: " + feature.properties.id;
    }
	if (text)
		layer.bindPopup(text);
}