package main

import (
	"context"
	"encoding/json"
	"errors"
	"net/http"
	"strconv"

	"go.mongodb.org/mongo-driver/bson"
)

type Body struct {
	Action string
	Usr    string
	Sum    string
	Data   []int32
	Tag    string
	Idx    string
}

func NewBody(r *http.Request) (*Body, error) {
	var body *Body
	err := json.NewDecoder(r.Body).Decode(&body)
	if err != nil {
		return nil, err
	}
	return body, nil
}

func (body *Body) authenticate() (*User, error) {
	var user User
	err := collection.FindOne(context.TODO(), bson.M{"usr": body.Usr}).Decode(&user)
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

func (body *Body) read(query *User) *string {
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
	res, err := collection.UpdateOne(context.TODO(), filter, push)
	if err != nil {
		return nil, err
	} else {
		msg := "\"Modified Count: " + strconv.Itoa(int(res.ModifiedCount)) + "\""
		return &msg, nil
	}
}

func (body *Body) delete(query *User) (*string, error) {
	// Delete by idx first, then tag
	filter := bson.M{"usr": body.Usr}

	var foundErr bool
	modified_sum := 0
	if body.Idx != "" {
		arr_idx := "secrets." + body.Idx
		unset := bson.D{{"$unset", bson.D{{arr_idx, 0}}}}
		if res, err := collection.UpdateOne(context.TODO(), filter, unset); err != nil {
			foundErr = true
		} else {
			modified_sum += int(res.ModifiedCount)
		}

		pull := bson.D{{"$pull", bson.D{{"secrets", nil}}}}
		if _, err := collection.UpdateOne(context.TODO(), filter, pull); err != nil {
			foundErr = true
		}
	}

	if body.Tag != "" {
		pull := bson.D{{"$pull", bson.D{{"secrets", bson.D{{"tag", body.Tag}}}}}}
		if res, err := collection.UpdateOne(context.TODO(), filter, pull); err != nil {
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

func (body *Body) create(query *User) (*string, error) {
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
	if _, err := collection.InsertOne(context.TODO(), newUser); err != nil {
		return nil, err
	} else {
		msg := "\"Created new user: " + body.Usr + "\""
		return &msg, nil
	}
}
