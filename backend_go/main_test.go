package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"math/rand"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

func TestHelloHandler(t *testing.T) {
	request, _ := http.NewRequest("GET", "/hello", nil)
	response := httptest.NewRecorder()
	HelloHandler(response, request)

	if response.Code != 200 {
		t.Errorf("Expected status: %v, got: %v\n", 200, response.Code)
	}
	if response.Body.String() != "Hello" {
		t.Errorf("Expected body: %v, got: %v\n", "Hello", response.Body)
	}
}

func testCreateUsr(t *testing.T, usrname string, sum string) {
	body := Body{"c", usrname, sum, nil, "", ""}
	b, _ := json.Marshal(body)

	// Make sure usr does not exist
	collection.DeleteOne(context.TODO(), bson.D{{Key: "usr", Value: body.Usr}})

	// Creating a new usr should pass, since the usr does not exist
	request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response := httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 200 {
		t.Errorf("Expected status: %v, got: %v\n", 200, response.Code)
	}
	//fmt.Printf("Response Body: %v\n", response.Body)

	// Creating a new usr should fail, since the usr already exists
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 401 {
		t.Errorf("Expected status: %v, got: %v\n", 401, response.Code)
	}
	//fmt.Printf("Response Body: %v\n", response.Body)

}

type Response struct {
	Res []Secret
	E   string
}

func parseResponse(r *httptest.ResponseRecorder) (*Response, error) {
	var res *Response
	err := json.NewDecoder(r.Body).Decode(&res)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func secretsMatch(s Secret, enc []int32, tag string) bool {
	if len(s.Enc) != len(enc) {
		return false
	}
	if s.Tag != tag {
		return false
	}
	for i, c := range s.Enc {
		if c != enc[i] {
			return false
		}
	}
	return true
}

func testCoreUsr(t *testing.T, usrname string, sum string) {
	// Read should work, results should be empty
	body := Body{"r", usrname, sum, nil, "", ""}
	b, _ := json.Marshal(body)
	request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response := httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 200 {
		t.Errorf("Expected status: %v, got: %v\n", 200, response.Code)
	}

	res, err := parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	if len(res.Res) != 0 {
		t.Errorf("Res should be empty: %v\n", res)
	}

	// Anything with a GET should fail
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("GET", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 405 {
		t.Errorf("Expected status: %v, got: %v\n", 405, response.Code)
	}

	// Writing without data should not work
	body = Body{"w", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 400 {
		t.Errorf("Expected status: %v, got: %v\n", 400, response.Code)
	}

	// Generate random write data
	numWrites := 5
	writes := make([][]int32, numWrites)
	for i := 0; i < numWrites; i++ {
		len := (rand.Int() % 63) + 1
		for j := 0; j < len; j++ {
			writes[i] = append(writes[i], int32(rand.Uint32()%255))
		}
	}

	// Write with data
	body = Body{"w", usrname, sum, writes[0], "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 200 {
		t.Errorf("Expected status: %v, got: %v\n", 200, response.Code)
	}

	// Read it back
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 200 {
		t.Errorf("Expected status: %v, got: %v\n", 200, response.Code)
	}

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	if len(res.Res) != 1 {
		t.Errorf("Res should be have len: %v, res = %v\n", 1, res)
	}
	if !secretsMatch(res.Res[0], writes[0], "") {
		t.Errorf("Data does not match")
	}
}

func testNoPwd(t *testing.T) {
	usrname := "___GO___TEST___USR___"
	testCreateUsr(t, usrname, "")
	testCoreUsr(t, usrname, "")

}

func testPwd(t *testing.T) {
	usrname := "___GO___TEST___USR___"
	sum := fmt.Sprintf("%v", rand.Uint64())
	testCreateUsr(t, usrname, sum)
	testCoreUsr(t, usrname, sum)
}

func TestUsrHandler(t *testing.T) {
	// Set up a DB connection
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	client, err := mongo.Connect(ctx, options.Client().ApplyURI(uri))
	if err != nil {
		t.Errorf("Problem accessing to DB from unit tests: %v/n", err)
	}
	defer func() {
		if err = client.Disconnect(ctx); err != nil {
			t.Errorf("Problem accessing to DB from unit tests: %v/n", err)
		}
	}()
	collection = client.Database("secrets").Collection("users")

	testNoPwd(t)
	testPwd(t)
}
