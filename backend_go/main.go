package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"

	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

var collection *mongo.Collection

func UsrHandler(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case "POST":
		body, err := NewBody(r)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}

		query, err := body.authenticate()
		if err != nil {
			http.Error(w, err.Error(), http.StatusUnauthorized)
			return
		}

		switch body.Action {
		case "r":
			res := body.read(query)
			fmt.Fprintf(w, "{\"Res\":%v}", *res)
		case "w":
			if res, err := write(body, query); err != nil {
				http.Error(w, err.Error(), http.StatusBadRequest)
			} else {
				fmt.Fprintf(w, "{\"Res\":%v}", *res)
			}
		case "d":
			if res, err := body.delete(query); err != nil {
				http.Error(w, err.Error(), http.StatusBadRequest)
			} else {
				fmt.Fprintf(w, "{\"Res\":%v}", *res)
			}
		case "c":
			if res, err := body.create(query); err != nil {
				http.Error(w, err.Error(), http.StatusBadRequest)
			} else {
				fmt.Fprintf(w, "{\"Res\":%v}", *res)
			}

		default:
			http.Error(w, "Action not recognized", http.StatusUnauthorized)
		}

	default:
		http.Error(w, "POST only", http.StatusMethodNotAllowed)
	}
}

type Secret struct {
	Enc []int32 `bson:"enc,omitempty"`
	Tag string  `bson:"tag,omitempty"`
}

type User struct {
	ID      primitive.ObjectID `bson:"_id,omitempty"`
	Usr     string             `bson:"usr,omitempty"`
	Sum     string             `bson:"sum,omitempty"`
	Secrets []Secret           `bson:"secrets,omitempty"`
}

func GetUri(path string) string {
	file, err := os.Open(path)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	scanner.Scan()
	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
	return scanner.Text()
}

func main() {
	args := os.Args
	if len(args) < 2 {
		panic("Missing argument: db authentication file")
	}
	uri := GetUri(args[1])

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	client, err := mongo.Connect(ctx, options.Client().ApplyURI(uri))
	if err != nil {
		panic(err)
	}
	defer func() {
		if err = client.Disconnect(ctx); err != nil {
			panic(err)
		}
	}()
	collection = client.Database("secrets").Collection("users")

	http.HandleFunc("/secrets/usr", UsrHandler)
	http.HandleFunc("/secrets/usr/", UsrHandler)
	fmt.Println("Running server")
	log.Fatal(http.ListenAndServe(":8000", nil))
}
