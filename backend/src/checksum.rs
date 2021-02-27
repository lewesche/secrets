use rand::prelude::*;
use std::num::Wrapping;

// These functions are used to generate a 
// "checksum" value from a two strings (ie
// username + password)

// Strings should be at least 3 char long
// "checksum" values are derived from two steps:

// 1) Add/multiply elements with overflow
// into a "hash". This seems to yield pretty
// random results, ie strings with the same
// straight forward additive sum will likely
// have a very different a "hash".
// However it's determinate and reversible.
pub fn hash_strings(s1: &String, s2: &String) -> u64 {
    let s1 = s1.as_bytes();

    let mut v1: Vec<Wrapping<u64>> = Vec::new(); 
    for i in 0..s1.len() {
        v1.push(Wrapping(s1[i].into()));
    }   

    let s2 = s2.as_bytes();
    let mut v2: Vec<Wrapping<u64>> = Vec::new(); 
    for i in 0..s2.len() {
        v2.push(Wrapping(s2[i].into()));
    }   
       
    let mut hash = Wrapping(123u64);
    // 110 is what I want because it's right in the middle of lower case chars in ascii
    let mut prev = Wrapping(110); 

    for e1 in v1.iter() {
        if e1 >= &prev {
            hash = hash * e1; 
        } else {
            hash = hash + e1; 
        }   
        prev = hash%Wrapping(255u64);
        if prev>=Wrapping(20u64) {prev -= Wrapping(20u64);}
        for e2 in v2.iter() {
            if e2 >= &prev {
                hash = hash * e2; 
            } else {
                hash = hash + e2;
            }
            prev = hash%Wrapping(255u64);
            if prev>=Wrapping(20u64) {prev -= Wrapping(20u64);}
        }
    }
    hash.0
}

// 2) Use the hash to seed a CSPRNG and take a random value.
// This random number is the "checksum". I'm no crypto
// expert, but I think the point of a CSPRNG is that it 
// isn't feasibly reversible. The Hc128 seems to perform
// well according to rust documentation
// https://docs.rs/rand/0.5.0/rand/prng/index.html
pub fn get_checksum(s1: &String, s2: &String) -> u128 {
    let hash = hash_strings(&s1, &s2);
    let mut rng = rand_hc::Hc128Rng::seed_from_u64(hash);
    rng.gen_range(0..u128::MAX)
}

