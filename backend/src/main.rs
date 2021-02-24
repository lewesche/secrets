#![feature(proc_macro_hygiene, decl_macro)]

use std::error::Error;
use std::fmt;

use rocket::{post, routes};
use json;
use std::process::Command;
use mongodb::{
    bson::{doc, Bson},
    sync::Client,
};

const LOCAL_DATA_PATH: &str = " -p /var/secrets_data/";
const DB_AUTH: &str = "mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority";

fn append_path(query: &mut String, name: String) {
    let mut usrpath = String::from(LOCAL_DATA_PATH);
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());
}

#[post("/secrets/usr", format = "application/json", data="<body>")]
fn query(body: String) -> String {
    let body = json::parse(body.as_str());
    let body = match body {
        Ok(body) => body,
        Err(_e) => return String::from("Bad json in post"),
    };

    let result = parse_query_body(body);
    let (query, user, pwd);

    match result {
        Ok(result) => { 
            query = result.0;
            user = result.1;
            pwd = result.2;
        },
        Err(e) => return format!("{}", e),
    }

    let result = db_lookup_user(&user, pwd, false);        
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
                        //println!("{}", query.as_str());

                        let output =Command::new("sh")
                            .arg("-c")
                            .arg(query.as_str())
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

#[derive(Debug)]
struct QueryError;

impl fmt::Display for QueryError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Querry Error")
    }
}

impl Error for QueryError {}

fn parse_query_body (body: json::JsonValue) -> Result<(String, String, Option<String>), QueryError> {
    let mut query = String::from("secrets -j");
    
    let a = body["action"].as_str();
    let a = match a {
        Some(a) => {
            query.push_str(" -"); 
            query.push_str(a);
            let data = body["data"].as_str();
            match data {
                Some(data) => {query.push_str(" "); query.push_str(data)},
                None => {if a=="w" { return Err(QueryError)}},
            }
            a // Looks funny but this overwrites the option a with a string a 
        },
        None => return Err(QueryError),
    };

    let k = body["key"].as_str();
    match k {
        Some(k) => {query.push_str(" -k "); query.push_str(k)},
        None => {if a=="w" || a=="r" {return Err(QueryError)}},
    }

    let i = body["idx"].as_str();
    match i {
        Some(i) => {query.push_str(" -i "); query.push_str(i)},
        None => (),
    }

    let t = body["tag"].as_str();
    match t {
        Some(t) => {query.push_str(" -t "); query.push_str(t)},
        None => (),
    }

    let user = body["usr"].as_str();
    let user = match user {
        Some(user) => {append_path(&mut query, String::from(user)); String::from(user)},
        None => return Err(QueryError),
    };

    let pwd = body["pwd"].as_str();
    let pwd = match pwd {
        Some(pwd) => Some(String::from(pwd)),
        None => None,
    };

    let result = (query, user, pwd);
    Ok(result)
}

#[post("/secrets/new", format = "application/json", data="<body>")]
fn create_user(body: String) -> String { 
    let body = json::parse(body.as_str());
    let body = match body {
        Ok(body) => body,
        Err(_e) => return String::from("Bad json in post"),
    };

    let result = parse_create_body(body);
    let (user, pwd);

    match result {
        Ok(result) => { 
            user = result.0;
            pwd = result.1;
        },
        Err(e) => return format!("{}", e),
    }

    let result = db_lookup_user(&user, pwd, true);        
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

fn parse_create_body (body: json::JsonValue) -> Result<(String, Option<String>), QueryError> {
    let user = body["usr"].as_str();
    let user = match user {
        Some(user) => String::from(user),
        None => return Err(QueryError),
    };

    let pwd = body["pwd"].as_str();
    let pwd = match pwd {
        Some(pwd) => Some(String::from(pwd)),
        None => None,
    };

    let result = (user, pwd);
    Ok(result)
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
                            if sum == pwd.as_str() {
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
        Some(pwd) => doc = doc! { "usr": usr.as_str(), "sum": pwd.as_str() },
        None => doc = doc! { "usr": usr.as_str() },
    }

    println!("new user: {}", doc);
    cli.database("secrets").collection("users").insert_one(doc, None)?;

    Ok(())
}

fn main() {
    rocket::ignite().mount("/", routes![create_user, query]).launch();
}

