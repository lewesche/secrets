#![feature(proc_macro_hygiene, decl_macro)]

use rocket::{get, routes};
use std::process::Command;
use mongodb::{
    bson::{doc, Bson},
    sync::Client,
};

const LOCAL_DATA_PATH: &str = " -p /var/secrets_data/";

fn append_path(query: &mut String, name: String) {
    let mut usrpath = String::from(LOCAL_DATA_PATH);
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());
}

#[get("/secrets/usr/<name>?<pwd>&<w>&<k>&<t>")]
fn write(name: String, pwd: Option<String>, w: String, k: String, t: Option<String>) -> String {
    let result = db_lookup_user(&name, pwd, false);        
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
    let mut query = String::from("secrets -w ");
    query.push_str(w.as_str());

    query.push_str(" -j -k ");
    query.push_str(k.as_str());
    
    match t {
        Some(t) => {query.push_str(" -t "); query.push_str(t.as_str())},
        None => (),
    }

    append_path(&mut query, name);

    // for debugging
    println!("{}", query.as_str());

    let output =Command::new("sh")
            .arg("-c")
            .arg(query.as_str())
            .output()
            .expect("failed to execute process");
    let data = output.stdout;
    
    format!("{}", String::from_utf8(data).unwrap())
                    }
                },
            }
        },
    }
}

#[get("/secrets/usr/<name>?<pwd>&r&<k>&<t>&<i>", rank=1)]
fn read(name: String, pwd: Option<String>, k: String, t: Option<String>, i: Option<String>) -> String {
    let result = db_lookup_user(&name, pwd, false);        
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
                        let mut query = String::from("secrets -r -j -k ");
                        query.push_str(k.as_str());
    
                        match t {
                            Some(t) => {query.push_str(" -t "); query.push_str(t.as_str())},
                            None => (),
                        }

                        match i {
                            Some(i) => {query.push_str(" -i "); query.push_str(i.as_str())},
                            None => (),
                        }

                        append_path(&mut query, name);

                        // for debugging
                        println!("{}", query.as_str());

                        let output =Command::new("sh")
                            .arg("-c")
                            .arg(query.as_str())
                            .output()
                            .expect("failed to execute process");
                        let data = output.stdout;
    
                        format!("{}", String::from_utf8(data).unwrap())
                    }
                },
            }
        },
    }
}

#[get("/secrets/usr/<name>?<pwd>&d&<t>&<i>")]
fn delete(name: String, pwd: Option<String>, t: Option<String>, i: Option<String>) -> String {
    let result = db_lookup_user(&name, pwd, false);        
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
    let mut query = String::from("secrets -d");

    match t {
        Some(t) => {query.push_str(" -t "); query.push_str(t.as_str())},
        None => (),
    }

    match i {
        Some(i) => {query.push_str(" -i "); query.push_str(i.as_str())},
        None => (),
    }

    append_path(&mut query, name);

    // for debugging
    println!("{}", query.as_str());

    let output =Command::new("sh")
            .arg("-c")
            .arg(query.as_str())
            .output()
            .expect("failed to execute process");
    let data = output.stdout;
    
    format!("{}", String::from_utf8(data).unwrap())
                    }
                },
            }
        },
    }
}



#[get("/secrets/new?<name>&<pwd>")]
fn create_user(name: String, pwd: Option<String>) -> String { 
    let result = db_lookup_user(&name, pwd, true);        
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
    let client = Client::with_uri_str("mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority")?;
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
    //let client = Client::with_uri_str("mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority")?;

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
    /*
    let usr =String::from("testusr_password");
    let pwd = String::from("1234");

    let result = db_check_user(&usr, Some(&pwd), false);        
    match result {
        Ok(val) => { 
            match val {
                UserStatus::Exists(auth) => println!("exists, authenticated: {}", auth),
                UserStatus::DoesNotExist => println!("does not exist"),
            }
        }
        Err(e) => {
            println!("database error: {}", e);
        }
    }

    let new_usr =String::from("new_testusr");
    let new_pwd = String::from("5678");
    db_check_user(&new_usr, Some(&new_pwd), true); 

    let another_usr =String::from("another_testusr");
    db_check_user(&another_usr, None, true); 
    */
    
    rocket::ignite().mount("/", routes![read, write, delete, create_user]).launch();
}






