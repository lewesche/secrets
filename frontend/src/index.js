module.exports = {
	init, 
	clicked_new_user,
	clicked_go, 
	wasm_test
}

const rust = import('../pkg');
const MAX_LEN = 4096;

function checkLen(obj) {
	if (JSON.stringify(obj).length > MAX_LEN) {
        throw "Request is too long"
    }
}

function wasm_test() {
	rust.then(m => {
		console.log('checksum: {}', m.get_checksum('username', 'password'))
		let enc = m.encode("username", "password", "hello")
		console.log('enc: {}', enc)
		let dec = m.decode("username", "password", enc)
		dec = String.fromCharCode(...dec)
		console.log('dec, {}', dec)
  }).catch(console.error);
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
    diagram.src="img/secrets_diagram_crop.png";
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

function new_user_pwd(usr, pwd) {
	rust.then(m => {
		body.sum = m.get_checksum(body.usr, pwd)
		checkLen(body);
		send(body);
  	})
}

function new_user_no_pwd () {
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

    clear_fields();

    if(pwd != "") {
		query_pwd(body, pwd)
	} else {
		query_no_pwd(body)
	}
}

function query_pwd(body, pwd) {
	rust.then(m => {
		body.sum = m.get_checksum(body.usr, pwd)

		if(body.action=="w") {
		    data = document.getElementById('dec_txt').value;
    		if(data !="") 
           		body.data = m.encode(body.usr, pwd, data);
		}

		checkLen(body);
		send(body);
  	})
}

function query_no_pwd(body) {
	if(body.action=="w") {
	    data = document.getElementById('dec_txt').value;
    	if(data !="")
        	body.data = data;
	}	
	checkLen(body);
	send(body);
}

function send(body) {
    let url = "/secrets/usr";
    console.log(body);

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
}

function parse_response(body, res) {
	console.log(res)
	let obj = JSON.parse(res);

	if(obj.success=="false") {
		if(obj.hasOwnProperty("e")) 
    		throw obj.e
    } else {
    	throw "Unknown Error"
    }

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
			if (decode) {
        		html_txt += "<td>" + m.decode(usr, pwd, obj[i].enc) + "</td>";
			} else {
				html_txt += "<td>" + obj[i].enc + "</td>";
			}
	        html_txt += "</tr>"
    	}
	    html_txt += "</table>";
    	document.getElementById("table_div").innerHTML = html_txt;
  	})
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


