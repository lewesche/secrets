import { checkLen, send, setError, clear } from "./util"

const rust = import('../../pkg');

// Without this, can't stringify 64 bit nums
BigInt.prototype.toJSON = function() { return this.toString(); };

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

export class newUserButton {
	constructor() {
		this.newUserButton = document.getElementById("new_usr_button");
		this.newUserButton.onclick = this.clickedNewUser;
	}

    clickedNewUser() {
        try {
            clear();

            let usr = document.getElementById('usr_txt').value;
            if (usr=="") {
                throw "Missing user!"
            }
            let body = {};
            body.action = "c";
            body.usr = usr;
            
            let pwd = document.getElementById('pwd_txt').value;
            if(pwd != "") {
                new_user_pwd(body, pwd)
            } else {
                new_user_no_pwd(body)
            }
        } catch (e) {
            setError(e)
        }
    }
}