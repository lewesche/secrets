# secrets
A simple tool for storing passwords or anything else you'd like to encrypt - without the risk of your data leaking. No "master password" data is ever stored. No "secrets" are stored in an unencrypted form.

The project started as a CLI tool written in C++. I'm currently working on a browser version in Rust, which will operate slightly differently and currently is in somehwat of a in between state. 

# Browser App

The browser app is running here: [secrets](https://lewesche.com/secrets.html)

The browser app requires a <username, password> pair for authenticaiton, but only the username is stored. Usernames + passwords are used to compute a "checksum" which cannot be reversed into the original password. Checksums are computed via a two step process. First, character values from the two strings are linearly combined into a single 64 bit value. Next this 64 bit value is used to seed a CSPRNG. The first value taken from the random number generator is the checksum. 

It's still using the C++ program to handle encryption/decryption. The CLI tool uses keys (different than passwords) to encrypt/decrypt. Each "secret" stored can have a different key. Keys are used to encrpyt/decrypt via a caeser cipher. 

The plan is to rebuild the caeser cipher part in Rust to 1) drop the use of keys to simplify the application with just the password, 2) incorperate CSPRNG random numberrs in the encryption process and 3) speed up the program.  

# CLI version
This is the first version of the project. Currently the browser version still uses the CLI tool to encrypt/decypt.

When you want to store a new "secret", you provide a key that is used to encrypt the secret phrase. When you want to read back a secret, provide the same key to decript it. 

## CLI Installation

### With Homebrew **currently broken
```
brew tap lewesche/secrets
brew install secrets
```

### Build yourself
Clone this repo, build the executable with "make", and move the executable to somewhere on the $PATH. 

Setup might look something like:
```
git clone ...
cd secrets
make
cp bin/secrets /usr/local/bin
```
Then run from anywhere with `secrets`

Before compiling with `make`, there are macros you can change within secret.h to modify the encryption/decryption algorithm in your binary. See the file for details. 

## CLI Usage
The first time it's run a file will be created in your home directory called `.secrets.txt`. This is where encrypted text and optionally unencrypted tags are stored. It might be a good idea to save a backup copy of this file. 

### h : show all commands
### r : read all secrets
- Use the same key to read everything. If you use different keys for different secrets, some will return gibberish. 
- The index and tag are displayed first, and followed by the decrypted secret on the next line. 
- Ex:
```
enter command (r/w/f/l/d/p/q/h): r
enter a key: password123
0     My netflix password
      netflix_passwrd
1     top secret
      ??D?G??#8뚀
```

### w : write new secret
- You'll be prompted for a key, the secret you want to save, and optionally a tag. 
- Ex:
```
enter command (r/w/f/l/d/p/q/h): w 
enter a key: key123 
enter phrase to encrypt: my_netflix_password
optionally, enter a non-integer tag: netflix
```
### f : find and read secret(s) by tag or index
- Indicies must be integers. They correspond to the order the secrets are stored, and may change as secrets are deleted. 
- Tags are anything that isn't an integer. 
- Ex:
```
enter command (r/w/f/l/d/p/q/h): f 
enter tag/index: top secret
enter a key: myKey
1     top secret
      roblox_password
```
###   l : list all secrets by index/tag
- Does not decrypt anything.
###   d : delete secret(s) by tag or index
- Does nothing if no matches are found.
###   p : use new path to secrets file
- By default, the file created in the home directory is used each time the program starts. With `p` you can specify a new file for the session.
- Creates the file specified if it's not found.
###   q : quit
