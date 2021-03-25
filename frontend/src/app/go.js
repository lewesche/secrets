import { checkLen, bytes, send, setError, clear } from "./util"
const rust = import('../../pkg');

// Without this, can't stringify 64 bit nums
BigInt.prototype.toJSON = function() { return this.toString(); };

function query(action) {
    let idx = document.getElementById('idx_txt').value;
    let tag = document.getElementById('tag_txt').value;
    let usr = document.getElementById('usr_txt').value;
    let pwd = document.getElementById('pwd_txt').value;

    let body = {}; 
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
                let data = document.getElementById('dec_txt').value;
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

function query_no_pwd(body) {
    if(body.action=="w") {
        let data = document.getElementById('dec_txt').value;
        if(data !="")
            body.data = bytes(data);
    }
    checkLen(body);
    send(body);
}

export class goButton {
	constructor() {
		this.goButton = document.getElementById("go_button");
		this.goButton.onclick = this.clickedGo;
	}
    
    clickedGo() {
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
	
}