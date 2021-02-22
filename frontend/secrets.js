
function clicked_new_user() {
	clear();
	usr = document.getElementById('usr_txt').value;
	pwd = document.getElementById('pwd_txt').value;

	let url = "/secrets/new?name=" + usr + "&pwd=" + pwd; 

	let xhr = new XMLHttpRequest();
	xhr.open("GET", url, true);
	xhr.onload = function (e) {
  		if (xhr.readyState === 4) {
    		if (xhr.status === 200) {
			let obj = JSON.parse(xhr.responseText);
			console.log(xhr.responseText);
			if(obj.success == "true") {
				setNotify("New user " + usr + " created")
			} else {
				setError(obj);
			}

    		} else {
      			console.error(xhr.statusText);
    		}
  		}
	};
	xhr.onerror = function (e) {
  		console.error(xhr.statusText);
	};
	xhr.send(null); 
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

	let url = "/secrets/usr/" + usr + "?" + action; 

	if(action=="w") {
		if(dec){
			url+= "=" + dec;
		} else {
			return;
		}
	}

	if(pwd != "") {
		url += "&pwd=" + pwd;
	}

	if(action=="r" || action=="w") 
		url += "&k=" + key;

	if(tag)
		url += "&t=" + tag;

	if(action!="w" && idx)
		url += "&i=" + idx;

	clear_fields();
	request(url, action);
}

function request(url) {
	console.log("url = " + url)

	let xhr = new XMLHttpRequest();
	xhr.open("GET", url, true);
	xhr.onload = function (e) {
  		if (xhr.readyState === 4) {
    		if (xhr.status === 200) {
				try {
					let obj = JSON.parse(xhr.responseText);
					console.log(xhr.responseText);
					if(!obj.hasOwnProperty("e")) {
						setTable(obj);
					} else {
						setError(obj);
					}

				} catch(e) { // valid delete and write responses return invalid json - prompting a recursive read
					send_query("r");
				}
    		} else {
      			console.error(xhr.statusText);
    		}
  		}
	};
	xhr.onerror = function (e) {
  		console.error(xhr.statusText);
	};
	xhr.send(null); 
}

function setNotify(str) {
	ele = document.getElementById("notify");
	ele.style.display = "block";
	ele.innerHTML = str;
}

function setError(obj) {
	ele = document.getElementById("error");
	ele.style.display = "block";
	ele.innerHTML = obj.e;
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
