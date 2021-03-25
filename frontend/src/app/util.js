const rust = import('../../pkg');

const MAX_LEN = 4096;

// Without this, can't stringify 64 bit nums
BigInt.prototype.toJSON = function() { return this.toString(); };

export const checkLen = (obj) => {
    if (JSON.stringify(obj).length > MAX_LEN) {
        throw "Request is too long"
    }
}

export const bytes = (s) => {
    let i, a = new Array(s.length);
    for (i = 0; i < s.length; i++) {
        a[i] = s.charCodeAt(i);
    }
    return a;
}

export const send = (body) => {
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
                    setError(xhr.responseText)
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
        if(obj.hasOwnProperty("e")) {
                throw obj.e
        }


        if(body.action == "r") {
            if ('sum' in body) {
                setTable(obj.Res, true);
            } else {
                setTable(obj.Res, false);
            }
        } else if (body.action == "c") {
            setNotify("New user " + body.usr + " created")
        } else {
            clear_fields()
		body.action = "r"
		body.tag = ""
		body.idx = ""
		send(body)
        }
    } catch (e) {
        setError(e)
    }
}


function setNotify(str) {
    let ele = document.getElementById("notify");
    ele.style.display = "block";
    ele.innerHTML = str;
}

export const setError = (e) => {
    let ele = document.getElementById("error");
    ele.style.display = "block";
    ele.innerHTML = e;
}

function setTable(obj, decode) {
    try {
    let usr = document.getElementById('usr_txt').value;
    let pwd = document.getElementById('pwd_txt').value;

    if(decode && (usr=="" || pwd=="")) {
        throw 'Missing username or password'
    }

    rust.then(m => {
        let html_txt = "<table>";
        html_txt += "<tr>" + "<th>idx</th>" + "<th>tag</th>" + "<th>text</th>" + "</tr>";
        for(let i=0; i<obj.length; i++) {
                html_txt += "<tr>"
            html_txt += "<td>" + i + "</td>";
            html_txt += "<td>";
                if(obj[i].hasOwnProperty('Tag')){
                html_txt += obj[i].Tag;
            }
                html_txt += "</td>";
        let dec;
        if (decode) {
            dec = m.decode(usr, pwd, obj[i].Enc)
        } else {
            dec = obj[i].Enc
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

export const clear = () => {
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
