function setCookie(name, value) {
	document.cookie = name + "=" + (value || "") + "; path=/";
}

function getCookie(name) {
	let nameEQ = name + "=";
	let ca = document.cookie.split(';');
	for (let i = 0; i < ca.length; i++) {
		let c = ca[i];
		while (c.charAt(0) == ' ') c = c.substring(1, c.length);
			if (c.indexOf(nameEQ) == 0)
				return c.substring(nameEQ.length, c.length);
	}
	return null;
}

function changeStyle(style) {
	setCookie("style", style);

	let refreshStyle = (document_) => {
		let style_tag = document_.getElementById("theme");
		if (style_tag)
			style_tag.setAttribute("href", style_tag.getAttribute("href"));
	};

	let frame = document.getElementsByTagName("iframe");
	console.log(frame[1]);

	refreshStyle(document);
	let frames = document.getElementsByTagName("iframe");
	for (let frame of frames)
		refreshStyle(frame.contentDocument);
}

function changeLanguage(lang) {
	setCookie("lang", lang);
	console.log(document.cookie);
	let it = location.href.indexOf('/', 12);
	window.location.href = location.href.substring(0, it + 1);
}