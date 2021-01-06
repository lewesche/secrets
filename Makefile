all: secrets

secrets: secrets.cpp
	g++ -Wall -g -std=c++11 secrets.cpp -o secrets

clean:
	rm secrets
