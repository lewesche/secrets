#![feature(proc_macro_hygiene, decl_macro)]
use std::io::Read;

use rocket::{Outcome, Request, post, Data, routes};
use rocket::Outcome::*;
use rocket::data::{self, FromDataSimple};
use rocket::http::{Status, ContentType};

use json;
use mongodb::{
    bson::{doc, Bson},
    sync::Client,
};

mod checksum;
use crate::checksum::get_checksum;
use crate::checksum::decode;
use crate::checksum::encode;

const DB_AUTH: &str = "mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority";

// DoS guard
const LIMIT: u64 = 4096;

enum Action {
    Read,
    Write,
    Delete,
    Create,
}

struct Query {
    user: String,
    pwd: Option<String>,
    action: Action,
    data: Option<String>,
    tag: Option<String>,
    idx: Option<usize>,
    result: String
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

        let action = body["action"].as_str();
        let action = match action {
            Some(action) => {
                match action {
                    "r" => Action::Read,
                    "w" => Action::Write,
                    "d" => Action::Delete,
                    "c" => Action::Create,
                    _ => return Failure((Status::InternalServerError, format!(": Action not recognized"))),
                }
            },
            None => return Failure((Status::InternalServerError, format!(": Missing action"))),
        };

        let data = body["data"].as_str();
        let data = match data {
            Some(data) => Some(String::from(data)),
            None => {
                match action {
                    Action::Write => {return Failure((Status::InternalServerError, format!(": Write is missing data")))},
                    _ => None,
                }
            },
        };

        let user = body["usr"].as_str();
        let user = match user {
            Some(user) => String::from(user),
            None => return Failure((Status::InternalServerError, format!(": Missing user"))),
        };

        let pwd = body["pwd"].as_str();
        let pwd = match pwd {
            Some(pwd) => Some(String::from(pwd)),
            None => None,
        };

        let idx = body["idx"].as_str();
        let idx = match idx {
            Some(idx) => {
                let idx = idx.parse::<usize>();
                let idx = match idx {
                    Ok(idx) => idx,
                    _ => return Failure((Status::InternalServerError, format!(": Invalid Idx")))
                };
                Some(idx)
            },
            None => None,
        };

        let tag = body["tag"].as_str();
        let tag = match tag {
            Some(tag) => Some(String::from(tag)),
            None => None,
        };

        let result = String::new();

        // Return successfully.
        Success(Query { user, pwd, action, data, tag, idx, result})
    }
}

enum QueryStatus {
    Fail(String),
    Success(String),
}

#[post("/secrets/usr", format = "application/json", data="<query>")]
fn query(query: Query) -> String {
    let status = db_query(&query);        
    match status {
        Err(e) => { 
            println!("Error: {}", e);
            format!("{{\"success\":\"false\", \"e\":\"{}\"}}", e)
        }, 
        Ok(status) => { 
            match status {
                QueryStatus::Fail(res) => format!("{{\"success\":\"false\", \"e\":\"{}\"}}", res), 
                QueryStatus::Success(res) => format!("{{\"success\":\"true\", \"res\":\"{}\"}}", res), 
            }
        },
    }
}

#[post("/secrets/new", format = "application/json", data="<query>")]
fn create_user(query: Query) -> String { 
    let status = db_query(&query);        
    match status {
        Err(e) => { 
            println!("Error: {}", e);
            format!("{{\"success\":\"false\", \"e\":\"{}\"}}", e)
        }, 
        Ok(status) => { 
            match status {
                QueryStatus::Fail(res) => format!("{{\"success\":\"false\", \"e\":\"{}\"}}", res), 
                QueryStatus::Success(res) => format!("{{\"success\":\"true\", \"res\":\"{}\"}}", res), 
            }
        },
    }
}

fn db_query(query: &Query) -> Result<QueryStatus, mongodb::error::Error> {
    let client = Client::with_uri_str(DB_AUTH)?;

    //Before anything, make a request to authenticate
    let lookup = client.database("secrets").collection("users").find_one(doc! { "usr": query.user.as_str() }, None)?;
    match lookup {
        Some(lookup) => {
            if let Some(sum) = lookup.get("sum").and_then(Bson::as_str) {
                match &query.pwd {
                    Some(pwd) => {
                        if sum == get_checksum(&query.user, &pwd).to_string() {
                            // passwords match
                            //return Ok(QueryStatus::Success(String::from("Found user, passwords match")))
                            return dispatch_query(&query, &client, &lookup, true)
                        } else {
                            // passwords do not match
                            return Ok(QueryStatus::Fail(String::from("Found user, passwords don't match")))
                        }
                    },
                    None => {
                        // no password provided, but one is expected
                        return Ok(QueryStatus::Fail(String::from("User expects a password, none provided")));
                    },  
                }
            } else {
                // no password required
                // return Ok(QueryStatus::Success(String::from("Found user, no password required")))
                //query.pwd = None;
                return dispatch_query(&query, &client, &lookup, false)
            }
        },
        None => {
            // no user found
            match query.action {
                Action::Create => db_create_user(&query, &client),
                _ => Ok(QueryStatus::Fail(String::from("No user found"))),
            }
        },
    }
}

fn db_create_user(query: &Query, client: &Client) -> Result<QueryStatus, mongodb::error::Error> {
    let doc;
    match &query.pwd {
        Some(pwd) => doc = doc! { "usr": query.user.as_str(), "sum": get_checksum(&query.user, &pwd).to_string() },
        None => doc = doc! { "usr": query.user.as_str() },
    }

    println!("new user: {}", doc);
    client.database("secrets").collection("users").insert_one(doc, None)?;

    Ok(QueryStatus::Success(String::from("New user created")))
}

fn dispatch_query(query: &Query, client: &Client, lookup: &mongodb::bson::Document, pwd_protected: bool) -> Result<QueryStatus, mongodb::error::Error> {
    match query.action {
       Action::Read => return db_read(&query, &lookup, pwd_protected),
       _ => Ok(QueryStatus::Fail(String::from("User already exists")))
    }
}

impl From<Secret> for json::JsonValue {
    fn from(s: Secret) -> Self {
        match s.tag {
            Some(tag) => json::object! {"tag" : tag, "enc" : String::from_utf8(s.data).unwrap()},
            None => json::object! {"enc": String::from_utf8(s.data).unwrap()},
        }
    }
}

#[derive(Clone)]
struct Secret {
    data: Vec<u8>,
    tag: Option<String>
}

// If thers is no password associated with the account, the user may still pass one in. Don't want
// to use that to decrypt, so this bool checks that. 
fn db_read(query: &Query, lookup: &mongodb::bson::Document, pwd_protected: bool) -> Result<QueryStatus, mongodb::error::Error> {
    let mut secrets = doc_2_secrets(&query, &lookup);
    secrets = filter(secrets, &query.tag, &query.idx);

    if pwd_protected {
        secrets = decode_secrets(secrets, &query.user, &query.pwd);
    } 

    Ok(QueryStatus::Success(String::from(json::stringify(secrets))))
}

fn doc_2_secrets(query: &Query, lookup: &mongodb::bson::Document) -> Vec<Secret> {
    let mut result: Vec<Secret> = Vec::new();
    
    // Read everything out of this annoying bson format
    
    let secrets = lookup.get("secrets");
    let secrets = match secrets {
        Some(secrets) => {
            secrets.as_array()
        },
        None => None,
    };

    match secrets {
        Some(secrets) => {
            //println!("{:?}", secrets);
            for s in secrets {
                let s = s.as_document();
                match s {
                    Some(s) => {
                        let tag = s.get("tag").and_then(Bson::as_str);
                        let tag = match tag {
                            Some(tag) => Some(String::from(tag)),
                            None => None,
                        };

                        let mut data: Vec<u8> = Vec::new();
                        let bson_enc = s.get("enc").and_then(Bson::as_array);
                        match bson_enc {
                            Some(bson_enc) => {
                                for e in bson_enc {
                                    let e = e.as_i32();
                                    match e {
                                        Some(e) => {
                                            data.push(e as u8);
                                        },
                                        None => ()
                                    }
                                }
                            },
                            None => ()
                        }
                        
                        result.push(Secret {data, tag});
                    },
                    None => ()
                }
            }
        },
        None => (),
    }
    result
    
    }


fn filter(secrets: Vec<Secret>, tag: &Option<String>, idx: &Option<usize>) -> Vec<Secret> {
    let mut filter_tag: bool = false;
    let tag = match tag {
        Some(tag) => {
            filter_tag = true;
            tag.clone()
        },
        None => String::new(),
    };

    let mut filter_idx: bool = false;
    let idx = match idx {
        Some(idx) => {
            filter_idx = true;
            println!("idx: {}", idx);
            *idx
        },
        None => 0,
    };

    if !filter_idx && !filter_tag {
        return secrets;
    }
    
    let mut result: Vec<Secret> = Vec::new();
    
    // apply filters (if any)
    for i in 0..secrets.len() {
        if filter_tag {
            match &secrets[i].tag {
                Some(temp_tag) => {
                    if *temp_tag == tag {
                        result.push(secrets[i].clone());
                        continue;
                    }
                },
                None => (),
            }
        }
        
        if filter_idx {
            if i == idx {
                result.push(secrets[i].clone());
            }
        }
        
    }
    result
}

fn decode_secrets(secrets: Vec<Secret>, user: &String, pwd: &Option<String>) -> Vec<Secret> {
    let mut result: Vec<Secret> = Vec::new();
    for s in secrets {
        let data = match pwd {
            Some(pwd) => decode(&user, &pwd, &s.data),
            None => { println!("This should be impossible..."); Vec::new()},
        };
        let tag = s.tag;
        result.push(Secret{data, tag});
    };

    result
}

fn main() {
    rocket::ignite().mount("/", routes![create_user, query]).launch();
}








