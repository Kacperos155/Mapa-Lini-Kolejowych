let main_url = location.origin;
const orginal_rt = document.getElementById('SearchResultTable').innerHTML;
let orginal_rd = document.getElementById('RouteDistanceTable').innerHTML;
let fromID = -1;
let from = "";
let toID = -1;
let to = "";

function select(id, type) {
	parent.postMessage('select;' + type + ';' + id, location.href);
}

function selectFromStation(id, name) {
	fromID = id;
	from = name;
	updateFromTo();
}

function selectToStation(id, name) {
	toID = id;
	to = name;
	updateFromTo();
}

function updateFromTo() {
	let table = document.getElementById('RouteDistance');
	if (fromID == -1 && toID == -1) {
		table.style.display = "none";
	}
	else {
		let rd = "";
		rd += '<tr class="result_table_row">';
		rd += '<th class="result_table">' + from + '</th>';
		rd += '<th class="result_table">' + to + '</th>';
		rd += '<th class="result_table">' + 'X km' + '</th>';
		rd += '</tr>';
		document.getElementById('RouteDistanceTable').innerHTML = orginal_rd + rd;
		table.style.display = "flex";
	};
}

function CalculateDistance() {
	if (fromID > -1 && toID > -1) {
		
		url = main_url + '/routing/' + fromID + '/' + toID;
		console.log(url);
		fetch(url)
			.then(response => response.json())
			.then((distance) => {
				let rd = "";
				rd += '<tr class="result_table_row">';
				rd += '<th class="result_table">' + from + '</th>';
				rd += '<th class="result_table">' + to + '</th>';
				rd += '<th class="result_table">' + distance +  ' m' + '</th>';
				rd += '</tr>';
				document.getElementById('RouteDistanceTable').innerHTML = orginal_rd + rd;
			}
			
	//bake the results
	orginal_rd = document.getElementById('RouteDistanceTable').innerHTML;
	fromID = -2;
	from = "";
	toID = -2;
	to = "";
	}
}

let search_box = document.getElementById("SearchText");
search_box.addEventListener("keyup", function (event) {
	if (event.keyCode === 13) {
		event.preventDefault();
		Search();
	}
});

let search_limit = 25;
function Search() {
	const type_radio_buttons = document.querySelectorAll('input[name="searchType"]');
	let type;

	for (const radio_button of type_radio_buttons) {
		if (radio_button.checked) {
			type = radio_button.value;
			break;
		}
	}
	if (!search_box.value)
		return;

	url = main_url + '/findElement/' + encodeURI(search_box.value) + '/' + type;
	if (search_limit != 5)
		url += '/' + search_limit;

	console.log(url);
	fetch(url)
		.then(response => response.json())
		.then((result) => {
			let table = document.getElementById('SearchResult');
			let no_result = document.getElementById('no_result');
			if (result) {
				let rt = "";
				for (const obj of result) {
					rt += '<tr class="result_table_row" onclick = "select(' + obj["id"] + ', ' + "'" + type + "'" + ')">';
					rt += '<th class="result_table">' + obj["id"] + '</th>';
					rt += '<th class="result_table">' + obj["name"] + '</th>';
					rt += '<th class="result_table" onclick = "selectFromStation(' + obj["id"] + ', ' + "'" + obj["name"] + "'" + ')">' + 'X' + '</th>';
					rt += '<th class="result_table" onclick = "selectToStation(' + obj["id"] + ', ' + "'" + obj["name"] + "'" + ')">' + 'X' + '</th>';
					rt += '</tr>';
				}
				document.getElementById('SearchResultTable').innerHTML = orginal_rt + rt;
				table.style.display = "flex";
				no_result.style.display = "none";
			}
			else {
				table.style.display = "none";
				no_result.style.display = "flex";
			}
		});
}
