#![feature(proc_macro_hygiene, decl_macro)]

use rocket::{get, routes};
use dirs::home_dir;
use std::process::Command;

fn get_path() -> String {
    let db_path = String::from(" -p /var/secrets_data/");
    return db_path;
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

    let mut usrpath = get_path();
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());

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

    let mut usrpath = get_path();
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());

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

    let mut usrpath = String::from(" -p ");
    match home_dir() {
        Some(path) => usrpath.push_str(&path.into_os_string().into_string().unwrap()),
        None => (),
    }

    match t {
        Some(t) => {query.push_str(" -t "); query.push_str(t.as_str())},
        None => (),
    }

    match i {
        Some(i) => {query.push_str(" -i "); query.push_str(i.as_str())},
        None => (),
    }

    let mut usrpath = get_path();
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());

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

    let mut usrpath = String::from(" -p ");
    match home_dir() {
        Some(path) => usrpath.push_str(&path.into_os_string().into_string().unwrap()),
        None => (),
    }

    match t {
        Some(t) => {query.push_str(" -t "); query.push_str(t.as_str())},
        None => (),
    }

    match i {
        Some(i) => {query.push_str(" -i "); query.push_str(i.as_str())},
        None => (),
    }
    
    let mut usrpath = get_path();
    usrpath.push_str(name.as_str());
    usrpath.push_str(".txt");
    query.push_str(usrpath.as_str());

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

fn main() {
    rocket::ignite().mount("/", routes![read, write, list, delete]).launch();
}






