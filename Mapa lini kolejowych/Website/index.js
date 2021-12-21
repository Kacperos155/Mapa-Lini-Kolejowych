window.addEventListener('message', (event) => {
	console.log("Recived message: ", event);
	if (event.origin == location.origin) {
		let map_target = document.getElementById("map_frame").contentWindow;
		if (map_target)
			map_target.postMessage(event.data, origin);
	}
})