module.exports = {
	init, 
	clicked_new_user,
	clicked_go, 
}

const rust = import('../pkg');
const MAX_LEN = 4096;

// Without this, can't stringify 64 bit nums
BigInt.prototype.toJSON = function() { return this.toString(); };

function checkLen(obj) {
	if (JSON.stringify(obj).length > MAX_LEN) {
        throw "Request is too long"
    }
}

// Sets up the little about section
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
	try {
    	clear();

    	usr = document.getElementById('usr_txt').value;
		if (usr=="") {
			throw "Missing user!"
		}
		let body = {};
		body.action = "c";
    	body.usr = usr;

		pwd = document.getElementById('pwd_txt').value;
		if(pwd != "") {
			new_user_pwd(body, pwd)
		} else {
			new_user_no_pwd(body)
		}
	} catch (e) {
		setError(e)
	}
}

function new_user_pwd(body, pwd) {
	try {
		rust.then(m => {
			body.sum = m.get_checksum(body.usr, pwd)
			checkLen(body);
			send(body);
	  	})
	} catch (e) {
		setError(e)
	}
}

function new_user_no_pwd(body) {
	checkLen(body);
	send(body);
}

function clicked_go() {
	try {
	    clear();
    	let radios = document.getElementsByName('action_button');
	    for(let i=0; i<radios.length; i++) {
    	    if(radios[i].checked) {
        	    query(radios[i].value)
            	return;
    	    }
	    }
	} catch (e) {
		setError(e);
	}
}

function query(action) {
    idx = document.getElementById('idx_txt').value;
    tag = document.getElementById('tag_txt').value;
    usr = document.getElementById('usr_txt').value;
    pwd = document.getElementById('pwd_txt').value;

    body = {};
    body.action = action;

    if(usr!="")
        body.usr = usr;
	
	if(action!="w" && idx!="")
        body.idx = idx;


    if(tag!="")
        body.tag = tag;

    if(pwd != "") {
		query_pwd(body, pwd)
	} else {
		query_no_pwd(body)
	}
}

function query_pwd(body, pwd) {
	try {
		rust.then(m => {
			body.sum = m.get_checksum(body.usr, pwd)

			if(body.action=="w") {
			    data = document.getElementById('dec_txt').value;
    			if(data !="") 
				// Leaving it as a uint8array causes json.stringify to get wonky
	           		body.data = Array.from(m.encode(body.usr, pwd, data));
			}

			checkLen(body);
			send(body);
  		})
	} catch (e) {
		setError(e)
	}
}

function bytes(s) {
	var i, a = new Array(s.length);
	for (i = 0; i < s.length; i++) {
		a[i] = s.charCodeAt(i);
	}
	return a;
}

function query_no_pwd(body) {
	if(body.action=="w") {
	    data = document.getElementById('dec_txt').value;
    	if(data !="")
        	body.data = bytes(data);
	}	
	checkLen(body);
	send(body);
}

function send(body) {
	try {
		let url = "/secrets/usr";
		let xhr = new XMLHttpRequest();
		xhr.open("POST", url, true);
		xhr.setRequestHeader('Content-Type', 'application/json; charset=UTF-8');
		xhr.onload = function (e) {
			if (xhr.readyState === 4) {
				if (xhr.status === 200) {
					parse_response(body, xhr.responseText);
				} else {
					throw "HTTP error: " + xhr.statusText
				}
			}
		};
		xhr.onerror = function (e) {
			throw "HTTP error: " + xhr.statusText
		};
		xhr.send(JSON.stringify(body))
	} catch (e) {
		setError(e)
	}
}

function parse_response(body, res) {
	try {
		let obj = JSON.parse(res);

		if(obj.success=="false") {
			if(obj.hasOwnProperty("e")) {
    				throw obj.e
			} else {
    				throw "Unknown Error"
    			}
		}


		clear_fields();

		if(body.action == "r") {
			if ('sum' in body) {
				setTable(obj.res, true);
			} else {
        		setTable(obj.res, false);
			}
		} else if (body.action == "c") {
			setNotify("New user " + body.usr + " created")
		} else {
	    		query("r");
		}
	} catch (e) {
		setError(e)
	}

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

function setTable(obj, decode) {
	try {
	usr = document.getElementById('usr_txt').value;
	pwd = document.getElementById('pwd_txt').value;
	
	if(decode && (usr=="" || pwd=="")) {
		throw 'Missing username or password'
	}

	rust.then(m => {
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
		    	let dec;
			if (decode) {
				dec = m.decode(usr, pwd, obj[i].enc)
			} else {
				dec = obj[i].enc
			}
		    	let asString = String.fromCharCode(...dec); 
			html_txt += "<td>" + asString + "</td>";
	        html_txt += "</tr>"
    	}
	    html_txt += "</table>";
    	document.getElementById("table_div").innerHTML = html_txt;
  	})
	} catch (e) {
		setError(e)
	}

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


