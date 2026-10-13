"use strict";

/*
 * Extras of this fork (ESP32 only): sound with a DFPlayer Mini, the animations
 * of Manfred Hofmann's Wortuhr_mp3, events and the BME280 sensor.
 *
 * The settings are stored by the web server on port 81 (/api/extras). If it
 * cannot be reached (e.g. ESP8266 firmware), all extras stay hidden.
 */
(function() {

	// Named sounds of the Wortuhr_mp3 sound package (folder "mp3" on the SD card)
	const SOUND_GROUPS = [
		{ group: "Signale", sounds: [[700, "OK"], [701, "Magical"], [702, "Startup 1"], [703, "Startup 2"], [705, "Alarm"], [706, "Ticker"], [750, "Müllabfuhr"], [800, "Fail"], [801, "Neues Item"], [802, "Impact"], [803, "Slide"], [804, "Slide Impact"], [805, "Pong"], [806, "Snake"], [807, "Success"]] },
		{ group: "Geburtstag", random: 3001, sounds: [[720, "Happy Birthday"], [721, "Geburtstag 2"], [722, "Geburtstag 3"], [723, "Geburtstag 4 (Marilyn)"], [724, "Geburtstag 5"], [725, "Geburtstag 6 (Zuckowski)"], [726, "Geburtstag 7 (Stevie Wonder)"]] },
		{ group: "Weihnachten", random: 3002, sounds: [[710, "Merry Christmas"], [711, "Coca-Cola Christmas"], [712, "Jingle Bells"], [713, "We Wish You A Merry Christmas"], [714, "Kling, Glöckchen"], [715, "O Tannenbaum"], [716, "Ihr Kinderlein, kommet"], [717, "Fröhliche Weihnacht"], [718, "Alle Jahre wieder"], [719, "Stille Nacht"]] },
		{ group: "Silvester", random: 3003, sounds: [[730, "ABBA – Happy New Year"], [731, "Feuerwerk (VW) 1"], [732, "Feuerwerk (VW) 2"], [733, "Feuerwerk"]] },
		{ group: "Natur", random: 3004, sounds: [[761, "Vögel 1"], [762, "Vögel 2"], [763, "Vögel 3"], [764, "Urwald 1"], [765, "Urwald 2"], [766, "Urwald 3"], [767, "Meeresrauschen 1"], [768, "Meeresrauschen 2"], [769, "Meeresrauschen 3"]] },
		{ group: "Tiere", random: 3005, sounds: [[811, "Krähe"], [812, "Schwan"], [813, "Bär"], [814, "Biene"], [815, "Frosch"], [816, "Kuh"], [817, "Vogel"], [818, "Hahn"], [819, "Hund"], [820, "Pferd"], [821, "Delfin"], [822, "Katze"], [823, "Elefant"], [824, "Schwein"], [825, "Kuckuck"], [826, "Schaf"], [827, "Möwe"], [828, "Wolf"], [829, "Eule"], [830, "Specht"], [831, "Affe"], [832, "Esel"], [833, "Klapperschlange"], [834, "Falke"], [835, "Wal"], [836, "Ente"], [837, "Löwe"], [838, "Taube"], [839, "Ratte"]] },
		{ group: "Musik", random: 3006, sounds: [[841, "Beethoven 5te"], [842, "Cocker Leave Your"], [843, "Bocelli Say Goodbye"], [844, "Scorpions Winds Change"], [845, "Kaoma Lambada"], [846, "Haddaway What Is Love"], [847, "Jürgens – Griechischer Wein"], [848, "Aqua Barbie Girl"], [849, "Schilling Major Tom"], [850, "Hasselhoff Freedom"], [851, "Nena Luftballons"], [852, "Falco Amadeus"], [853, "Fanta Die Da"], [854, "Kiss I Was Made"], [855, "Coldplay Clocks"], [856, "Madonna Prayer"], [857, "Europe Final Countdown"], [858, "Angels Daylight"], [859, "Rosenberg Marleen"], [860, "Ramstein Engel"], [861, "Spice Girls Wannabe"], [862, "Purple Smoke Water"], [863, "Beatles Submarine"], [864, "Acdc Highway Hell"], [865, "Queen Rock You"], [866, "Mey – Über den Wolken"], [867, "Fischer Atemlos"], [868, "Lavigne Skater"], [869, "Dion Heart Will Go On"], [871, "ABBA – Mamma Mia"], [872, "ABBA – Dancing Queen"], [873, "ABBA – Knowing Me"], [874, "ABBA – Take A Chance"], [875, "ABBA – Lay Your Love"], [876, "ABBA – Super Trouper"], [877, "ABBA – Have A Dream"], [878, "ABBA – Winner"], [879, "ABBA – Money"], [880, "ABBA – Sos"], [881, "ABBA – Chiquitita"], [882, "ABBA – Fernando"], [883, "ABBA – Voulez Vous"], [884, "ABBA – Gimme"], [885, "ABBA – Mother"], [886, "ABBA – One Of Us"], [887, "ABBA – Name Game"], [888, "ABBA – Thank For Music"], [889, "ABBA – Waterloo"], [890, "ABBA – Honey"], [891, "ABBA – I Do"], [892, "ABBA – Hasta Manana"], [893, "ABBA – Head Over Heels"], [894, "ABBA – Ring Ring"], [895, "ABBA – Me And I"], [896, "ABBA – Our Last Summer"], [897, "ABBA – Kissed Teacher"]] }
	];

	const REPEAT_OPTIONS = [0, 15, 30, 60, 120, 180, 240];
	const STATUS_INTERVAL_MS = 20000;

	let extrasData = null;
	let saveTimer = null;

	function byId(id) {
		return document.getElementById(id);
	}

	function tr(key) {
		return i18next.t(key);
	}

	function trList(key) {
		return i18next.t(key, { returnObjects: true });
	}

	// The extras live on the clock's second web server (port 81).
	function serverUrl(path) {
		const host = isLocalEnvironment() ? "192.168.2.167" : location.hostname;
		return `http://${host}:81${path}`;
	}

	function apiUrl(path) {
		return serverUrl(`/api/extras${path}`);
	}

	function showToast(text) {
		let toast = byId("extras-toast");
		if (!toast) {
			toast = document.createElement("div");
			toast.id = "extras-toast";
			toast.className = "extras-toast";
			document.body.appendChild(toast);
		}
		toast.textContent = text;
		toast.classList.add("visible");
		clearTimeout(toast.hideTimer);
		toast.hideTimer = setTimeout(() => toast.classList.remove("visible"), 1800);
	}

	async function apiGet(path) {
		const response = await fetch(apiUrl(path));
		return response.json();
	}

	async function apiPost(path, body) {
		// text/plain avoids a CORS preflight request
		const response = await fetch(apiUrl(path), {
			method: "POST",
			headers: { "Content-Type": "text/plain" },
			body: JSON.stringify(body)
		});
		return response.json();
	}

	async function apiCall(path, okText) {
		try {
			const response = await fetch(path.startsWith("/setEvent") ? serverUrl(path) : apiUrl(path));
			const result = await response.json();
			showToast(result.ok ? (okText || "OK") : (result.error || tr("extras.error")));
		} catch (error) {
			console.debug("extras:", error);
			showToast(tr("extras.offline"));
		}
	}

	/*
	 * Helpers for building <select> elements
	 */

	function addOption(select, value, text, selected) {
		const option = document.createElement("option");
		option.value = value;
		option.textContent = text;
		if (selected) {
			option.selected = true;
		}
		select.appendChild(option);
		return option;
	}

	function animationLabel(name) {
		const time = /^Z(\d\d)(\d\d)$/.exec(name);
		if (time) {
			return `${time[1]}:${time[2]} – ${tr("extras.hour.time-animations").split(" (")[0]}`;
		}
		const text = name.toLowerCase().replace(/_/g, " ").replace(/muell/g, "müll");
		return text.charAt(0).toUpperCase() + text.slice(1);
	}

	function fillAnimationSelect(select, selected, withNone) {
		select.innerHTML = "";
		if (withNone) {
			addOption(select, "", tr("events.fields.none"), !selected);
		}
		extrasData.animations.forEach(name => {
			addOption(select, name, animationLabel(name), name === selected);
		});
	}

	function fillSoundSelect(select, selected, withNone) {
		select.innerHTML = "";
		let found = !selected;
		if (withNone) {
			addOption(select, "0", tr("events.fields.none"), !selected);
		}
		SOUND_GROUPS.forEach(group => {
			const optgroup = document.createElement("optgroup");
			optgroup.label = group.group;
			if (group.random) {
				// the clock picks a random sound of this group each time
				addOption(optgroup, String(group.random), `🎲 ${tr("events.fields.random")} – ${group.group}`, group.random === selected);
				if (group.random === selected) {
					found = true;
				}
			}
			group.sounds.forEach(sound => {
				const option = addOption(optgroup, String(sound[0]), sound[1], sound[0] === selected);
				if (sound[0] === selected) {
					found = true;
					option.selected = true;
				}
			});
			select.appendChild(optgroup);
		});
		if (!found) {
			addOption(select, String(selected), `Nr. ${selected}`, true);
		}
	}

	function fillHourSelect(select, selected) {
		select.innerHTML = "";
		for (let hour = 0; hour < 24; hour++) {
			addOption(select, String(hour), `${String(hour).padStart(2, "0")}:00`, hour === selected);
		}
	}

	/*
	 * Settings
	 */

	function collectSettings() {
		const settings = Object.assign({}, extrasData.settings);

		settings.chime = byId("hour-chime").checked;
		settings.singleGong = byId("hour-chime-single").checked;
		settings.voice24h = byId("hour-chime-24h").checked;
		const chimeSound = Number(byId("hour-chime-sound").value);
		if (chimeSound >= 0) {
			settings.chimeSet = [0, 1, 2, 3, 4, 5, 6].map(() => chimeSound);
		} else {
			settings.chimeSet = [0, 1, 2, 3, 4, 5, 6].map(day => Number(byId(`hour-chime-day${day}`).value));
		}
		settings.hourAnimation = byId("hour-animation").value;

		settings.events = byId("events-enabled").checked;

		settings.sound = byId("sound-enabled").checked;
		settings.volume = Number(byId("sound-volume").value);
		settings.volumeByTime = byId("sound-volume-by-time").checked;
		settings.weekendLater = byId("sound-weekend-later").checked;
		settings.quiet = byId("sound-quiet").checked;
		settings.quietFrom = Number(byId("sound-quiet-from").value);
		settings.quietTo = Number(byId("sound-quiet-to").value);
		settings.voice = Number(byId("sound-voice").value);
		settings.startupSound = byId("sound-startup").checked;
		settings.rxPin = Number(byId("sound-rx-pin").value);
		settings.txPin = Number(byId("sound-tx-pin").value);
		settings.busyPin = Number(byId("sound-busy-pin").value);

		settings.altitude = Number(byId("climate-altitude").value);
		settings.temperatureOffset = Number(byId("climate-offset").value);
		return settings;
	}

	function saveSettingsSoon() {
		clearTimeout(saveTimer);
		saveTimer = setTimeout(async() => {
			try {
				const settings = collectSettings();
				const result = await apiPost("/settings", settings);
				if (result.ok) {
					extrasData.settings = settings;
				}
				showToast(result.ok ? tr("extras.saved") : (result.error || tr("extras.error")));
			} catch (error) {
				console.debug("extras:", error);
				showToast(tr("extras.offline"));
			}
		}, 400);
	}

	function renderWeekdays(chimeSet) {
		const container = byId("hour-chime-weekdays");
		container.innerHTML = "";
		const weekdays = trList("events.weekdays");
		// Monday first, Sunday last
		[1, 2, 3, 4, 5, 6, 0].forEach(day => {
			const group = document.createElement("div");
			group.className = "pure-control-group";
			const label = document.createElement("label");
			label.htmlFor = `hour-chime-day${day}`;
			label.textContent = weekdays[day];
			const select = document.createElement("select");
			select.id = `hour-chime-day${day}`;
			extrasData.soundSets.forEach((name, index) => addOption(select, String(index), name, chimeSet[day] === index));
			select.addEventListener("change", saveSettingsSoon);
			group.appendChild(label);
			group.appendChild(select);
			container.appendChild(group);
		});
	}

	function updateHourVisibility() {
		const chimeOn = byId("hour-chime").checked;
		document.querySelectorAll(".hour-chime-option").forEach(el => el.classList.toggle("extras-collapsed", !chimeOn));
		const perWeekday = Number(byId("hour-chime-sound").value) < 0;
		byId("hour-chime-weekdays").classList.toggle("extras-collapsed", !chimeOn || !perWeekday);
	}

	function renderSettings() {
		const settings = extrasData.settings;

		// full hour (box "transition")
		byId("hour-chime").checked = settings.chime;
		byId("hour-chime-single").checked = settings.singleGong;
		byId("hour-chime-24h").checked = settings.voice24h;
		const sameEveryDay = settings.chimeSet.every(value => value === settings.chimeSet[0]);
		const chimeSelect = byId("hour-chime-sound");
		chimeSelect.innerHTML = "";
		extrasData.soundSets.forEach((name, index) => addOption(chimeSelect, String(index), name, sameEveryDay && settings.chimeSet[0] === index));
		addOption(chimeSelect, "-1", tr("extras.hour.per-weekday"), !sameEveryDay);
		renderWeekdays(settings.chimeSet);

		const animationSelect = byId("hour-animation");
		animationSelect.innerHTML = "";
		addOption(animationSelect, "", tr("extras.hour.off"), settings.hourAnimation === "");
		addOption(animationSelect, "Z", tr("extras.hour.time-animations"), settings.hourAnimation === "Z");
		addOption(animationSelect, "*", tr("extras.hour.random"), settings.hourAnimation === "*");
		extrasData.animations.filter(name => !/^Z\d{4}$/.test(name)).forEach(name => {
			addOption(animationSelect, name, animationLabel(name), settings.hourAnimation === name);
		});
		updateHourVisibility();

		// events
		byId("events-enabled").checked = settings.events;

		// sound (settings)
		byId("sound-enabled").checked = settings.sound;
		byId("sound-volume").value = settings.volume;
		byId("sound-volume-value").textContent = settings.volume;
		byId("sound-volume-by-time").checked = settings.volumeByTime;
		byId("sound-weekend-later").checked = settings.weekendLater;
		byId("sound-quiet").checked = settings.quiet;
		fillHourSelect(byId("sound-quiet-from"), settings.quietFrom);
		fillHourSelect(byId("sound-quiet-to"), settings.quietTo);
		byId("sound-voice").value = settings.voice;
		byId("sound-startup").checked = settings.startupSound;
		byId("sound-rx-pin").value = settings.rxPin;
		byId("sound-tx-pin").value = settings.txPin;
		byId("sound-busy-pin").value = settings.busyPin;

		// room climate
		byId("climate-altitude").value = settings.altitude;
		byId("climate-offset").value = settings.temperatureOffset;
	}

	function renderStatus() {
		const status = extrasData.status;
		let player = tr("extras.sound.missing");
		if (status.dfplayer) {
			player = status.playing ? tr("extras.sound.playing") : tr("extras.sound.ready");
		}
		byId("sound-player-state").value = player;

		const climate = status.climate || {};
		const hasSensor = climate.sensor && climate.sensor !== "-";
		byId("climate-sensor").value = hasSensor ? climate.sensor : tr("extras.climate.none");
		document.querySelectorAll(".climate-only").forEach(el => el.classList.toggle("extras-collapsed", !hasSensor));
		byId("climate-temperature").value = climate.temperature != null ? `${climate.temperature.toFixed(1)} °C` : "–";
		byId("climate-humidity").value = climate.humidity != null ? `${Math.round(climate.humidity)} %` : "–";
		byId("climate-pressure").value = climate.pressure != null ? `${Math.round(climate.pressure)} hPa` : "–";
	}

	/*
	 * Events
	 */

	function labelled(text, control, wide) {
		const label = document.createElement("label");
		label.className = wide ? "event-field event-field-wide" : "event-field";
		const span = document.createElement("span");
		span.textContent = text;
		label.appendChild(span);
		label.appendChild(control);
		return label;
	}

	function inputElement(type, value, attributes) {
		const input = document.createElement("input");
		input.type = type;
		input.value = value;
		Object.keys(attributes || {}).forEach(key => input.setAttribute(key, attributes[key]));
		return input;
	}

	function eventCard(eventData) {
		const card = document.createElement("div");
		card.className = "event-card";

		const day = inputElement("number", eventData.day, { min: 1, max: 31 });
		day.dataset.key = "day";

		const month = document.createElement("select");
		month.dataset.key = "month";
		trList("events.months").forEach((name, index) => addOption(month, String(index + 1), name, eventData.month === index + 1));

		const year = inputElement("number", eventData.year || "", { min: 1900, max: 2999, placeholder: tr("events.fields.year-placeholder") });
		year.dataset.key = "year";

		const hh = String(eventData.hour).padStart(2, "0");
		const mm = String(eventData.minute).padStart(2, "0");
		const time = inputElement("time", `${hh}:${mm}`, {});
		time.dataset.key = "time";

		const endHour = eventData.endHour == null ? 23 : eventData.endHour;
		const endMinute = eventData.endMinute == null ? 59 : eventData.endMinute;
		const endTime = inputElement("time", `${String(endHour).padStart(2, "0")}:${String(endMinute).padStart(2, "0")}`, {});
		endTime.dataset.key = "end";

		const repeat = document.createElement("select");
		repeat.dataset.key = "interval";
		const options = REPEAT_OPTIONS.indexOf(eventData.interval) >= 0 ? REPEAT_OPTIONS : REPEAT_OPTIONS.concat([eventData.interval]);
		options.forEach(minutes => {
			const text = minutes === 0 ? tr("events.fields.once") : `${tr("events.fields.every")} ${minutes} ${tr("events.fields.minutes")}`;
			addOption(repeat, String(minutes), text, eventData.interval === minutes);
		});

		const text = inputElement("text", eventData.text, { maxlength: 80 });
		text.dataset.key = "text";

		const sound = document.createElement("select");
		sound.dataset.key = "sound";
		fillSoundSelect(sound, eventData.sound, true);

		const pre = document.createElement("select");
		pre.dataset.key = "pre";
		fillAnimationSelect(pre, eventData.pre, true);

		const post = document.createElement("select");
		post.dataset.key = "post";
		fillAnimationSelect(post, eventData.post, true);

		const grid = document.createElement("div");
		grid.className = "event-grid";
		grid.appendChild(labelled(tr("events.fields.day"), day));
		grid.appendChild(labelled(tr("events.fields.month"), month));
		grid.appendChild(labelled(tr("events.fields.year"), year));
		grid.appendChild(labelled(tr("events.fields.time"), time));
		grid.appendChild(labelled(tr("events.fields.repeat"), repeat));
		const endField = labelled(tr("events.fields.until"), endTime);
		grid.appendChild(endField);
		// an end time only makes sense for repeated events
		const updateEnd = () => endField.classList.toggle("extras-collapsed", Number(repeat.value) === 0);
		repeat.addEventListener("change", updateEnd);
		updateEnd();
		grid.appendChild(labelled(tr("events.fields.text"), text, true));
		grid.appendChild(labelled(tr("events.fields.sound"), sound));
		grid.appendChild(labelled(tr("events.fields.pre"), pre));
		grid.appendChild(labelled(tr("events.fields.post"), post));
		card.appendChild(grid);

		const actions = document.createElement("div");
		actions.className = "extras-buttons";
		const testButton = document.createElement("button");
		testButton.type = "button";
		testButton.className = "pure-button button-secondary";
		testButton.textContent = tr("events.fields.test");
		testButton.addEventListener("click", () => {
			const values = readEventCard(card);
			let message = values.text;
			if (values.year > 1) {
				message = message.replace("YY", String(new Date().getFullYear() - values.year));
			}
			apiCall(`/setEvent?text=${encodeURIComponent(message)}&audio=${values.sound}` +
				`&preani=${encodeURIComponent(values.pre)}&postani=${encodeURIComponent(values.post)}`);
		});
		const deleteButton = document.createElement("button");
		deleteButton.type = "button";
		deleteButton.className = "pure-button button-danger";
		deleteButton.textContent = tr("events.fields.delete");
		deleteButton.addEventListener("click", () => {
			card.remove();
			renderEmptyHint();
		});
		actions.appendChild(testButton);
		actions.appendChild(deleteButton);
		card.appendChild(actions);
		return card;
	}

	function readEventCard(card) {
		const values = {};
		card.querySelectorAll("[data-key]").forEach(el => {
			values[el.dataset.key] = el.value;
		});
		const timeParts = (values.time || "08:00").split(":");
		const endParts = (values.end || "23:59").split(":");
		return {
			day: Number(values.day),
			month: Number(values.month),
			year: Number(values.year || 0),
			hour: Number(timeParts[0]),
			minute: Number(timeParts[1]),
			endHour: Number(endParts[0]),
			endMinute: Number(endParts[1]),
			interval: Number(values.interval),
			text: values.text,
			sound: Number(values.sound),
			pre: values.pre,
			post: values.post
		};
	}

	function renderEmptyHint() {
		const list = byId("events-list");
		const hint = list.querySelector(".events-empty");
		const hasCards = list.querySelector(".event-card") !== null;
		if (!hasCards && !hint) {
			const paragraph = document.createElement("p");
			paragraph.className = "events-empty";
			paragraph.textContent = tr("events.list.empty");
			list.appendChild(paragraph);
		} else if (hasCards && hint) {
			hint.remove();
		}
	}

	function renderEvents() {
		const list = byId("events-list");
		list.innerHTML = "";
		const sorted = extrasData.events.slice().sort((first, second) => (first.month * 100 + first.day) - (second.month * 100 + second.day));
		sorted.forEach(eventData => list.appendChild(eventCard(eventData)));
		renderEmptyHint();
	}

	async function saveEvents() {
		const events = [...byId("events-list").querySelectorAll(".event-card")].map(readEventCard);
		try {
			const result = await apiPost("/events", events);
			if (result.ok) {
				extrasData.events = events;
			}
			showToast(result.ok ? tr("extras.saved") : (result.error || tr("extras.error")));
		} catch (error) {
			console.debug("extras:", error);
			showToast(tr("extras.offline"));
		}
	}

	async function importEvents(file) {
		try {
			const json = JSON.parse(await file.text());
			const result = await apiPost("/events", json);
			if (result.ok) {
				await loadExtras();
			}
			showToast(result.ok ? tr("extras.saved") : (result.error || tr("extras.error")));
		} catch (error) {
			console.debug("extras:", error);
			showToast(tr("extras.error"));
		}
	}

	/*
	 * "Animations & announcements" and "Show a message now"
	 */

	function renderPlayControls() {
		fillAnimationSelect(byId("play-animation"), "", false);
		fillSoundSelect(byId("play-sound"), 720, false);
		fillSoundSelect(byId("message-sound"), 0, true);
		fillAnimationSelect(byId("message-pre"), "HERZ", true);
		fillAnimationSelect(byId("message-post"), "", true);
	}

	function setupEventHandlers() {
		document.querySelectorAll(".extras-setting").forEach(el => {
			el.addEventListener("change", () => {
				updateHourVisibility();
				saveSettingsSoon();
			});
		});
		byId("hour-chime-sound").addEventListener("change", () => {
			const value = Number(byId("hour-chime-sound").value);
			if (value >= 0) {
				[0, 1, 2, 3, 4, 5, 6].forEach(day => {
					byId(`hour-chime-day${day}`).value = String(value);
				});
			}
			updateHourVisibility();
			saveSettingsSoon();
		});
		byId("sound-volume").addEventListener("input", () => {
			byId("sound-volume-value").textContent = byId("sound-volume").value;
		});

		byId("hour-chime-test").addEventListener("click", () => apiCall("/announce?what=chime"));
		byId("play-animation-button").addEventListener("click", () => {
			apiCall(`/animation?name=${encodeURIComponent(byId("play-animation").value)}`);
		});
		byId("play-sound-button").addEventListener("click", () => apiCall(`/play?track=${byId("play-sound").value}`));
		document.querySelectorAll(".announce-button").forEach(button => {
			button.addEventListener("click", () => apiCall(`/announce?what=${button.value}`));
		});
		byId("play-stop-button").addEventListener("click", () => apiCall("/stop"));
		byId("sound-reset-button").addEventListener("click", () => apiCall("/dfreset"));

		byId("events-add-button").addEventListener("click", () => {
			const today = new Date();
			byId("events-list").appendChild(eventCard({
				day: today.getDate(),
				month: today.getMonth() + 1,
				year: 0,
				hour: 8,
				minute: 0,
				endHour: 21,
				endMinute: 0,
				interval: 0,
				text: "",
				sound: 0,
				pre: "",
				post: ""
			}));
			renderEmptyHint();
		});
		byId("events-save-button").addEventListener("click", saveEvents);
		byId("events-import").addEventListener("change", event => {
			const file = event.target.files[0];
			if (file) {
				importEvents(file);
			}
			event.target.value = "";
		});

		byId("message-button").addEventListener("click", () => {
			apiCall(`/setEvent?text=${encodeURIComponent(byId("message-text").value)}` +
				`&audio=${byId("message-sound").value}` +
				`&preani=${encodeURIComponent(byId("message-pre").value)}` +
				`&postani=${encodeURIComponent(byId("message-post").value)}`);
		});
	}

	function renderAll() {
		renderSettings();
		renderStatus();
		renderEvents();
		renderPlayControls();
	}

	async function loadExtras() {
		extrasData = await apiGet("");
		renderAll();
	}

	async function refreshStatus() {
		try {
			const data = await apiGet("");
			extrasData.status = data.status;
			renderStatus();
		} catch (error) {
			// the clock is restarting or offline; try again later
			console.debug("extras: status", error);
		}
	}

	document.addEventListener("DOMContentLoaded", async() => {
		try {
			await loadExtras();
		} catch (error) {
			// No extras on this clock (e.g. ESP8266): keep everything hidden.
			console.debug("extras: not available", error);
			return;
		}
		document.querySelectorAll(".extras-only").forEach(el => el.classList.remove("extras-only"));
		setupEventHandlers();
		setInterval(refreshStatus, STATUS_INTERVAL_MS);
		// texts depend on the language
		i18next.on("languageChanged", renderAll);
	});
}());
