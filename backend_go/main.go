package main

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

const uri = "mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority"

type Body struct {
	Action string
	Usr    string
	Sum    string
	Data   []int32
	Tag    string
	Idx    string
}

func parseBody(r *http.Request) (*Body, error) {
	var body *Body
	err := json.NewDecoder(r.Body).Decode(&body)
	if err != nil {
		return nil, err
	}
	return body, nil
}

func authenticate(body *Body) (*User, error) {
	var user User
	err := collection.FindOne(nil, bson.M{"usr": body.Usr}).Decode(&user)
	if err != nil {
		if body.Action != "c" {
			return nil, errors.New("User not found")
		}
	} else {
		if body.Action == "c" {
			return nil, errors.New("User already exists")
		}
	}
	if user.Sum != "" {
		if body.Sum == "" {
			return nil, errors.New("Missing Password")
		} else if body.Sum != user.Sum {
			return nil, errors.New("Wrong Password")
		}
	}
	return &user, nil
}

func read(body *Body, query *User) *string {
	var res []Secret

	tag_filter := false
	if body.Tag != "" {
		tag_filter = true
	}

	idx_filter := false
	if body.Idx != "" {
		idx_filter = true
	}

	i := 0
	for _, secret := range query.Secrets {
		if tag_filter {
			if secret.Tag == body.Tag {
				res = append(res, secret)
				i++
				continue
			}
		}
		if idx_filter {
			if body.Idx == strconv.Itoa(i) {
				res = append(res, secret)
				i++
				continue
			}
		}
		if !tag_filter && !idx_filter {
			res = append(res, secret)
		}
		i++
	}
	resJson, _ := json.Marshal(res)
	resJsonString := string(resJson)
	if resJsonString == "null" {
		resJsonString = "[]"
	}
	return &resJsonString
}

func write(body *Body, query *User) (*string, error) {
	if len(body.Data) == 0 {
		return nil, errors.New("No data to write")
	}

	filter := bson.M{"usr": body.Usr}
	var push bson.D
	if body.Tag == "" {
		push = bson.D{{"$push", bson.D{{"secrets", bson.D{{"enc", body.Data}}}}}}
	} else {
		push = bson.D{{"$push", bson.D{{"secrets", bson.D{{"tag", body.Tag}, {"enc", body.Data}}}}}}
	}
	res, err := collection.UpdateOne(nil, filter, push)
	if err != nil {
		return nil, err
	} else {
		msg := "\"Modified Count: " + strconv.Itoa(int(res.ModifiedCount)) + "\""
		return &msg, nil
	}
}

func delete(body *Body, query *User) (*string, error) {
	// Delete by idx first, then tag
	filter := bson.M{"usr": body.Usr}

	var foundErr bool
	modified_sum := 0
	if body.Idx != "" {
		arr_idx := "secrets." + body.Idx
		unset := bson.D{{"$unset", bson.D{{arr_idx, 0}}}}
		if res, err := collection.UpdateOne(nil, filter, unset); err != nil {
			foundErr = true
		} else {
			modified_sum += int(res.ModifiedCount)
		}

		pull := bson.D{{"$pull", bson.D{{"secrets", nil}}}}
		if _, err := collection.UpdateOne(nil, filter, pull); err != nil {
			foundErr = true
		}
	}

	if body.Tag != "" {
		pull := bson.D{{"$pull", bson.D{{"secrets", bson.D{{"tag", body.Tag}}}}}}
		if res, err := collection.UpdateOne(nil, filter, pull); err != nil {
			foundErr = true
		} else {
			modified_sum += int(res.ModifiedCount)
		}
	}
	if foundErr {
		return nil, errors.New("Something went wrong in delete")
	} else {
		msg := "\"Modified Count: " + strconv.Itoa(modified_sum) + "\""
		return &msg, nil
	}
}

func create(body *Body, query *User) (*string, error) {
	var newUser bson.D
	if body.Sum == "" {
		newUser = bson.D{
			{Key: "usr", Value: body.Usr},
			{Key: "secrets", Value: bson.A{}},
		}
	} else {
		newUser = bson.D{
			{Key: "usr", Value: body.Usr},
			{Key: "sum", Value: body.Sum},
			{Key: "secrets", Value: bson.A{}},
		}
	}
	if _, err := collection.InsertOne(nil, newUser); err != nil {
		return nil, err
	} else {
		msg := "\"Created new user: " + body.Usr + "\""
		return &msg, nil
	}
}

func usrHandler(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case "POST":
		body, err := parseBody(r)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}

		query, err := authenticate(body)
		if err != nil {
			http.Error(w, err.Error(), http.StatusUnauthorized)
			return
		}

		switch body.Action {
		case "r":
			res := read(body, query)
			fmt.Fprintf(w, "{\"Res\":%v}", *res)
		case "w":
			if res, err := write(body, query); err != nil {
				http.Error(w, err.Error(), http.StatusBadRequest)
			} else {
				fmt.Fprintf(w, "{\"Res\":%v}", *res)
			}
		case "d":
			if res, err := delete(body, query); err != nil {
				http.Error(w, err.Error(), http.StatusBadRequest)
			} else {
				fmt.Fprintf(w, "{\"Res\":%v}", *res)
			}
		case "c":
			if res, err := create(body, query); err != nil {
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

var collection *mongo.Collection

func main() {
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

	http.HandleFunc("/secrets/usr", usrHandler)
	fmt.Println("Running server")
	log.Fatal(http.ListenAndServe(":8000", nil))
}
