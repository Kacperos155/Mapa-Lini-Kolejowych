let main_url = location.origin;
const orginal_rt = document.getElementById('SearchResultTable').innerHTML;

function select(id, type) {
	parent.postMessage('select;' + type + ';' + id, location.href);
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
					rt += '<tr class="result_table_row" onclick = "select(';
					rt += obj["id"] + ', ' + "'" + type + "'" + ')">';
					rt += '<th class="result_table">' + obj["id"] + '</th>';
					rt += '<th class="result_table">' + obj["name"] + '</th>';
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
