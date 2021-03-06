const MAX_LEN = 4096;

function init() {
	let about = document.getElementById('about');
        let title = document.createElement("h2");

        title.innerHTML = "- About";
        title.classList.add("left", "dropDown");
        about.append(title);

	let inner = document.createElement("div");
	
	let p1 = document.createElement("p");
	p1.innerHTML = "Secrets is an app that lets you store strings like passwords in a secure way."
	inner.append(p1);

	let p2 = document.createElement("p");
	p2.innerHTML = "No passwords are saved, and no data is saved in an unencrypted format."
	inner.append(p2);	
	
	let p3 = document.createElement("p");
	p3.innerHTML = "Optionally, secrets can be created with a tag."
	inner.append(p3);	

	let p4 = document.createElement("p");
	p4.innerHTML = "Tags, along with the index, can be used to filter read/delete operations. "
	inner.append(p4);	


	let diagram = document.createElement("img");
	diagram.src="secrets_diagram_crop.png";
	diagram.width = "950";
	inner.append(diagram);


        about.append(inner);

	$(".dropDown").click(function(){
		let curr = this.innerHTML;
		if(curr[0] == '-') {
			curr = curr.replace('-', '+');
		} else if (curr[0] == '+') {
			curr = curr.replace('+', '-');
		}
		this.innerHTML = curr;
		var oldWidth = $(this.nextSibling).width();
		$(this.nextSibling).slideToggle(200, () => {$(this).width(oldWidth);} );
	});

}

function clicked_new_user() {
	clear();
	usr = document.getElementById('usr_txt').value;
	pwd = document.getElementById('pwd_txt').value;

	body = {};
	body.action = "c";
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
							setTable(obj.res);
						} else {
							// Read again to get an updated table
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
		html_txt += "<td>" + i + "</td>";	
		html_txt += "<td>";	
		if(obj[i].hasOwnProperty('tag')){
			html_txt += obj[i].tag;	
		}
		html_txt += "</td>";	

		html_txt += "<td>" + obj[i].enc + "</td>";	
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
	document.getElementById('idx_txt').value = "";
	document.getElementById('tag_txt').value = "";
	document.getElementById('dec_txt').value = "";
}
