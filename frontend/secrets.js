const MAX_LEN = 4096;

function clicked_new_user() {
	clear();
	usr = document.getElementById('usr_txt').value;
	pwd = document.getElementById('pwd_txt').value;

	body = {};
	body.usr = usr;
	if(pwd != "")
		body.pwd = pwd;

	if (JSON.stringify(body).length > MAX_LEN) {
		clear();
		setError("Request is too long")
		return;
	}


	let url = "/secrets/new"; 

	let xhr = new XMLHttpRequest();
	xhr.open("POST", url, true);
	xhr.setRequestHeader('Content-Type', 'application/json; charset=UTF-8');
	xhr.onload = function (e) {
  		if (xhr.readyState === 4) {
    		if (xhr.status === 200) {
			console.log(xhr.responseText);
			let obj = JSON.parse(xhr.responseText);
			if(obj.success == "true") {
				setNotify("New user " + usr + " created")
			} else if(obj.hasOwnProperty("e")) {
				setError(obj.e);
			} else {
				setError("Failed with unknown error");
			}
    		} else {
			setError("HTTP error: " + xhr.statusText);
    		}
  		}
	};
	xhr.onerror = function (e) {
		setError("HTTP error: " + xhr.statusText);
	};
	xhr.send(JSON.stringify(body)); 
}

function clicked_go() {
	clear();
	let radios = document.getElementsByName('action_button');
	for(let i=0; i<radios.length; i++) {
		if(radios[i].checked) {
			send_query(radios[i].value)
			return;
		}
	}
}

function send_query(action) {
	key = document.getElementById('key_txt').value;
	idx = document.getElementById('idx_txt').value;
	tag = document.getElementById('tag_txt').value;
	usr = document.getElementById('usr_txt').value;
	dec = document.getElementById('dec_txt').value;
	pwd = document.getElementById('pwd_txt').value;

	body = {};

	body.action = action;

	if(action=="w")
		if(dec !="")
			body.data = dec;

	if(usr!="")
		body.usr = usr;

	if(pwd != "") 
		body.pwd = pwd;

	if(action=="r" || action=="w") 
		body.key = key;

	if(tag!="")
		body.tag = tag;

	if(action!="w" && idx!="")
		body.idx = idx;

	clear_fields();

	if (JSON.stringify(body).length > MAX_LEN) {
		setError("Request is too long")
		return;
	}

	request(body);
}

function request(body) {
	let url = "/secrets/usr"; 
	console.log(body);

	let xhr = new XMLHttpRequest();
	xhr.open("POST", url, true);
	xhr.setRequestHeader('Content-Type', 'application/json; charset=UTF-8');
	xhr.onload = function (e) {
  		if (xhr.readyState === 4) {
    		if (xhr.status === 200) {
				try {
					console.log(xhr.responseText);
					let obj = JSON.parse(xhr.responseText);					
					if(obj.success=="true") {
						if(body.action == "r") {
							setTable(obj.secrets);
						} else {
							send_query("r");
						}
					} else if(obj.hasOwnProperty("e")) {
						setError(obj.e);
					} else {
						setError("Unknown Error");
					}
				} catch(e) { // valid delete and write responses return invalid json - prompting a recursive read
					setError(e);
					console.log("bad json: " + xhr.responseText + e);
				}
    		} else {
			setError("HTTP error: " + xhr.statusText);
    		}
  		}
	};
	xhr.onerror = function (e) {
  		console.error(xhr.statusText);
		setError("HTTP error: " + xhr.statusText);
	};
	xhr.send(JSON.stringify(body)); 
}

function setNotify(str) {
	ele = document.getElementById("notify");
	ele.style.display = "block";
	ele.innerHTML = str;
}

function setError(e) {
	ele = document.getElementById("error");
	ele.style.display = "block";
	ele.innerHTML = e;
}

function setTable(obj) {
	console.log("in set table");
	html_txt = "<table>";
	html_txt += "<tr>" + "<th>idx</th>" + "<th>tag</th>" + "<th>text</th>" + "</tr>";
	for(i=0; i<obj.length; i++) {
		html_txt += "<tr>"
		html_txt += "<td>" + obj[i].idx + "</td>";	
		html_txt += "<td>";	
		if(obj[i].hasOwnProperty('tag')){
			html_txt += obj[i].tag;	
		}
		html_txt += "</td>";	

		html_txt += "<td>" + obj[i].dec + "</td>";	
		html_txt += "</tr>"
	}
	html_txt += "</table>";
	document.getElementById("table_div").innerHTML = html_txt;
}

function clear() {
	clearError();
	clearTable();
	clearNotify();
}

function clearNotify() {
	document.getElementById("notify").style.display = "none";
}

function clearError() {
	document.getElementById("error").style.display = "none";
}


function clearTable() {
	document.getElementById("table_div").innerHTML = "";
}

function clear_fields() {
	//document.getElementById('key_txt').value = "";
	document.getElementById('idx_txt').value = "";
	document.getElementById('tag_txt').value = "";
	document.getElementById('dec_txt').value = "";
}
