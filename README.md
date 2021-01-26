# secrets
Simple tool for storing passwords or anything else you'd like to encrypt. 

No "master password" data is ever stored. When you want to store a new "secret", you provide a key that is used to encrypt the secret phrase. When you want to read back a secret, provide the same key to decript it. 

## Installation

### With Homebrew
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

## Usage
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
