////let Legend_Collapsing = false;
////showLegend

let Legend_markers = [
	{
		label: "Station",
		type: "image",
		url: "/Icons/train.png"
	},
	{
		label: "Halt",
		type: "image",
		url: "/Icons/train_small.png"
	},
	{
		label: "Disused station/halt",
		type: "image",
		url: "/Icons/train_old.png"
	}
];

let Legend_speedLimits = [];

function addLegendSpeedLimit(label, color) {
	Legend_speedLimits.push(
		{
			label: label,
			type: "polyline",
			weight: 5,
			color: color
		});
}

addLegendSpeedLimit("Zoom is too low", defaultLineColor);
{
	let lastSpeedLimit = {};
	let label_prefix = "Speed limit" + ": ";
	for (let i = 0; i < (speedLimits.length - 1); ++i) {
		let label = label_prefix + speedLimits[i].min_speed + " - " + (speedLimits[i + 1].min_speed - 1) + " km/h";
		addLegendSpeedLimit(label, speedLimits[i].color);
	}
	let last = speedLimits[speedLimits.length - 1];
	addLegendSpeedLimit(label_prefix + "<= " + last.min_speed + " km/h", last.color);
}

let currentLegend = L.control.Legend();

function refreshLegend(speed_map) {
	currentLegend.remove();

	if (speed_map) {
		legends = Legend_markers.concat(Legend_speedLimits);
	}
	else {
		legends = Legend_markers.concat([
			{
				label: "Rail line",
				type: "polyline",
				weight: 5,
				color: defaultLineColor
			},
			{
				label: "Disused rail",
				type: "polyline",
				weight: 5,
				color: "rgb(0, 0, 0)"
			}
		]);
	}

	currentLegend = L.control.Legend({
		title: "Legend",
		position: "bottomleft",
		collapsed: !speed_map,
		column: legends.length / Legend_markers.length,
		symbolWidth: 32,
		symbolHeight: 32,
		legends: legends
	});
	currentLegend.addTo(map);
}

refreshLegend(false);