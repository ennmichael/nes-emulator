(function () {
	var country = {'code':'rs','id':63,'defaultLangauge':'rs'};
	
	if (!window.evidon) window.evidon = {};
	
	if (window.evidon.notice) {
		window.evidon.notice.setLocation(country);
	}
	else {
		window.evidon.location = country;
	}
})();