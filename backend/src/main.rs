#![feature(proc_macro_hygiene, decl_macro)]
use std::io::Read;

use rocket::{Outcome, Request, post, Data, routes};
use rocket::Outcome::*;
use rocket::data::{self, FromDataSimple};
use rocket::http::{Status, ContentType};

use json;
use std::process::Command;
use mongodb::{
    bson::{doc, Bson},
    sync::Client,
};

mod checksum;
use crate::checksum::get_checksum;

const LOCAL_DATA_PATH: &str = " -p /var/secrets_data/";
const DB_AUTH: &str = "mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority";

// DoS guard
const LIMIT: u64 = 4096;

fn append_path(query: &mut String, name: String) {
    let mut usrpath = String::from(LOCAL_DATA_PATH);
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());
}

struct Query {
    cmd: String,
    user: String,
    pwd: Option<String>
}

impl FromDataSimple for Query {
    type Error = String;

    fn from_data(req: &Request, data: Data) -> data::Outcome<Self, String> {
        // Ensure the content type is correct before opening the data.
        let query_ct = ContentType::parse_flexible("application/json");
        if req.content_type() != query_ct.as_ref() {
            return Outcome::Forward(data);
        }

        // Read the data into a String.
        let mut body = String::new();
        if let Err(e) = data.open().take(LIMIT).read_to_string(&mut body) {
            return Failure((Status::InternalServerError, format!("{:?}", e)));
        }
        
        let body = json::parse(body.as_str());
        let body = match body {
            Ok(body) => body,
            Err(e) => return Failure((Status::InternalServerError, format!("{:?}", e))),
        };

        let mut cmd = String::from("secrets -j");

        let a = body["action"].as_str();
        let a = match a {
            Some(a) => {
                cmd.push_str(" -"); 
                cmd.push_str(a);
                let data = body["data"].as_str();
                match data {
                    Some(data) => {cmd.push_str(" "); cmd.push_str(data)},
                    None => if a=="w" {return Failure((Status::InternalServerError, format!(": Write is missing data")))},
                }
                a // Looks funny but this overwrites the option a with a string a 
            },
            None => return Failure((Status::InternalServerError, format!(": Missing action"))),
        };

        let k = body["key"].as_str();
        match k {
            Some(k) => {cmd.push_str(" -k "); cmd.push_str(k)},
            None => if a=="w" || a=="r" { return Failure((Status::InternalServerError, format!(": Missing key")))},
        }

        let i = body["idx"].as_str();
        match i {
            Some(i) => {cmd.push_str(" -i "); cmd.push_str(i)},
            None => (),
        }

        let t = body["tag"].as_str();
        match t {
            Some(t) => {cmd.push_str(" -t "); cmd.push_str(t)},
            None => (),
        }

        let user = body["usr"].as_str();
        let user = match user {
            Some(user) => {append_path(&mut cmd, String::from(user)); String::from(user)},
            None => return Failure((Status::InternalServerError, format!(": Missing key"))),
        };

        let pwd = body["pwd"].as_str();
        let pwd = match pwd {
            Some(pwd) => Some(String::from(pwd)),
            None => None,
        };

        // Return successfully.
        Success(Query { cmd, user, pwd })
    }
}

#[post("/secrets/usr", format = "application/json", data="<body>")]
fn query(body: Query) -> String {
    let result = db_lookup_user(&body.user, body.pwd, false);        
    match result {
        Err(e) => { 
            println!("Error: {}", e);
            format!("{{\"success\":\"false\", \"e\":\"Error accessing database.\"}}")
        }, 
        Ok(val) => { 
            match val {
                UserStatus::Created => format!("{{\"success\":\"false\", \"e\":\"Unknown Error\"}}"),
                UserStatus::DoesNotExist => format!("{{\"success\":\"false\", \"e\":\"User does not exist!\"}}"),
                UserStatus::Exists(auth) => { 
                    if !auth {
                        format!("{{\"success\":\"false\", \"e\":\"Incorrect password\"}}")
                    } else {
                        // for debugging
                        //println!("{}", body.cmd.as_str());

                        let output =Command::new("sh")
                            .arg("-c")
                            .arg(body.cmd.as_str())
                            .output()
                            .expect("failed to execute process");
                        let data = output.stdout;
                        
                        let result = String::from_utf8(data).unwrap();
                        format!("{}",result)
                    }
                },
            }
        },
    }
}

struct NewUser {
    user: String,
    pwd: Option<String>
}

impl FromDataSimple for NewUser {
    type Error = String;

    fn from_data(req: &Request, data: Data) -> data::Outcome<Self, String> {
        // Ensure the content type is correct before opening the data.
        let query_ct = ContentType::parse_flexible("application/json");
        if req.content_type() != query_ct.as_ref() {
            return Outcome::Forward(data);
        }

        // Read the data into a String.
        let mut body = String::new();
        if let Err(e) = data.open().take(LIMIT).read_to_string(&mut body) {
            return Failure((Status::InternalServerError, format!("{:?}", e)));
        }
        
        let body = json::parse(body.as_str());
        let body = match body {
            Ok(body) => body,
            Err(e) => return Failure((Status::InternalServerError, format!("{:?}", e))),
        };

        let user = body["usr"].as_str();
        let user = match user {
            Some(user) => String::from(user),
            None => return Failure((Status::InternalServerError, format!(": Missing key"))),
        };

        let pwd = body["pwd"].as_str();
        let pwd = match pwd {
            Some(pwd) => Some(String::from(pwd)),
            None => None,
        };

        // Return successfully.
        Success(NewUser { user, pwd })
    }
}


#[post("/secrets/new", format = "application/json", data="<body>")]
fn create_user(body: NewUser) -> String { 
    let result = db_lookup_user(&body.user, body.pwd, true);        
    match result {
        Ok(val) => { 
            match val {
                UserStatus::Created => format!("{{\"success\":\"true\"}}"),
                UserStatus::Exists(_auth) => format!("{{\"success\":\"false\", \"e\":\"User already exists!\"}}"),
                UserStatus::DoesNotExist => format!("{{\"success\":\"false\", \"e\":\"Unknown Error\"}}"),
            }
        }
        Err(e) => { 
            println!("Error: {}", e);
            format!("{{\"success\":\"false\", \"e\":\"Error accessing database.\"}}")
        }, 
    }
}

enum UserStatus {
    Exists(bool),
    Created,
    DoesNotExist,
}

fn db_lookup_user(usr: &String, pwd: Option<String>, create: bool) -> Result<UserStatus, mongodb::error::Error> {
    let client = Client::with_uri_str(DB_AUTH)?;
    let result = client.database("secrets").collection("users").find_one(doc! { "usr": usr.as_str() }, None)?;
        match result {
            Some(document) => {
                if let Some(sum) = document.get("sum").and_then(Bson::as_str) {
                    match pwd {
                        Some(pwd) => {
                            if sum == get_checksum(&usr, &pwd).to_string() {
                            //if sum == pwd.as_str() {
                                return Ok(UserStatus::Exists(true))
                            } else {
                                return Ok(UserStatus::Exists(false))
                            }
                        },
                        None => {
                            return Ok(UserStatus::Exists(false));
                        },  
                    }
                } else {
                    return Ok(UserStatus::Exists(true))
                }
            },
            None => {
                if create {
                    db_create_user(&client, &usr, pwd)?;
                    return Ok(UserStatus::Created)
                }
            },
        }
    Ok(UserStatus::DoesNotExist)
}

fn db_create_user(cli: &Client, usr: &String, pwd: Option<String>) -> Result<(), mongodb::error::Error> {
    let doc;
    match pwd {
        Some(pwd) => doc = doc! { "usr": usr.as_str(), "sum": get_checksum(&usr, &pwd).to_string() },
        None => doc = doc! { "usr": usr.as_str() },
    }

    println!("new user: {}", doc);
    cli.database("secrets").collection("users").insert_one(doc, None)?;

    Ok(())
}

fn main() {
    rocket::ignite().mount("/", routes![create_user, query]).launch();
}

