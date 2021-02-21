#![feature(proc_macro_hygiene, decl_macro)]

use rocket::{get, routes};
use std::process::Command;
use mongodb::{
    bson::{doc, Bson},
    sync::Client,
};

const LOCAL_DATA_PATH: &str = "-p /var/secrets_data/";

fn append_path(query: &mut String, name: String) {
    let mut usrpath = String::from(LOCAL_DATA_PATH);
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());
}

#[get("/secrets/usr/<name>?<w>&<k>&<t>")]
fn write(name: String, w: String, k: String, t: Option<String>) -> String {
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

#[get("/secrets/usr/<name>?r&<k>&<t>&<i>", rank=1)]
fn read(name: String, k: String, t: Option<String>, i: Option<String>) -> String {
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

#[get("/secrets/usr/<name>?l&<t>&<i>", rank=2)]
fn list(name: String, t: Option<String>, i: Option<String>) -> String {
    let mut query = String::from("secrets -l -j");

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

#[get("/secrets/usr/<name>?d&<t>&<i>")]
fn delete(name: String, t: Option<String>, i: Option<String>) -> String {
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


fn test_mongo() -> Result<(), mongodb::error::Error> {
    //let client = Client::with_uri_str("mongodb://localhost:27017")?;
    //let database = client.database("mydb");
    //let collection = database.collection("books");

    println!("connecting to db");
    let client = Client::with_uri_str("mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority")?;
    for db_name in client.list_database_names(None, None)? {
        println!("Database: {}", db_name);
    }
    println!("done printing db names");
    Ok(())
}


fn main() {
    // Parse a connection string into an options struct.
    //let mut client_options = ClientOptions::parse("mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority").await?;

    // Manually set an option.
    //client_options.app_name = Some("My App".to_string());

    // Get a handle to the deployment.
    //let client = Client::with_options(client_options)?;

    // List the names of the databases in that deployment.
    //for db_name in client.list_database_names(None, None).await? {
    //    println!("{}", db_name);
    //}

    test_mongo();        

    rocket::ignite().mount("/", routes![read, write, list, delete]).launch();
}






