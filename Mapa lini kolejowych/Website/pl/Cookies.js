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
	//location.reload();

	let style_tag = document.getElementById("main_style");
	let x = style_tag.getAttribute("href");
	style_tag.setAttribute("href", x);
}