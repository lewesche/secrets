package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"math/rand"
	"net/http"
	"net/http/httptest"
	"os"
	"sync"
	"testing"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

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

func checkMatch(t *testing.T, s Secret, enc []int32, tag string) {
	if len(s.Enc) != len(enc) {
		t.Errorf("Read back len %v, expected %v\n", len(s.Enc), len(enc))
	}
	if s.Tag != tag {
		t.Errorf("Read back tag %v, expected %v\n", s.Tag, tag)
	}
	for i, c := range s.Enc {
		if c != enc[i] {
			t.Errorf("Read back enc %v, expected %v\n", s.Enc, enc)
		}
	}
}

func checkLen(t *testing.T, res []Secret, target int) {
	if len(res) != target {
		t.Errorf("Res should be have len: %v, len(res) = %v\n", target, len(res))
	}
}

func checkCode(t *testing.T, code int, target int) {
	if code != target {
		t.Errorf("Expected code: %v, got: %v\n", target, code)
	}
}

func testCoreUsr(t *testing.T, usrname string, sum string) {
	testCreateUsr(t, usrname, sum)

	// Read should work, results should be empty
	body := Body{"r", usrname, sum, nil, "", ""}
	b, _ := json.Marshal(body)
	request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response := httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err := parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 0)

	// Anything with a GET should fail
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("GET", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 405)

	// Writing without data should not work
	body = Body{"w", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 400)

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
	checkCode(t, response.Code, 200)

	// Read it back
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 1)
	checkMatch(t, res.Res[0], writes[0], "")

	tags := [2]string{"0", "1"}

	// Try writing with a tag
	body = Body{"w", usrname, sum, writes[1], tags[0], ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	// Read both back
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 2)
	checkMatch(t, res.Res[0], writes[0], "")
	checkMatch(t, res.Res[1], writes[1], tags[0])

	// Write more entires, two with tags and one without
	body = Body{"w", usrname, sum, writes[2], tags[1], ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	body = Body{"w", usrname, sum, writes[3], "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	body = Body{"w", usrname, sum, writes[4], tags[0], ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	if response.Code != 200 {
		t.Errorf("Expected status: %v, got: %v\n", 200, response.Code)
	}

	// Read back and validate all
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 5)
	checkMatch(t, res.Res[0], writes[0], "")
	checkMatch(t, res.Res[1], writes[1], tags[0])
	checkMatch(t, res.Res[2], writes[2], tags[1])
	checkMatch(t, res.Res[3], writes[3], "")
	checkMatch(t, res.Res[4], writes[4], tags[0])

	// Read by idx
	body = Body{"r", usrname, sum, nil, "", "3"}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 1)
	checkMatch(t, res.Res[0], writes[3], "")

	// Read by idx
	body = Body{"r", usrname, sum, nil, "", "3"}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 1)
	checkMatch(t, res.Res[0], writes[3], "")

	// Read by tag
	body = Body{"r", usrname, sum, nil, tags[1], ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 1)
	checkMatch(t, res.Res[0], writes[2], tags[1])

	// Read by tag with multiple matches
	body = Body{"r", usrname, sum, nil, tags[0], ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 2)
	checkMatch(t, res.Res[0], writes[1], tags[0])
	checkMatch(t, res.Res[1], writes[4], tags[0])

	// Read by tag + idx
	body = Body{"r", usrname, sum, nil, tags[1], "3"}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 2)
	checkMatch(t, res.Res[0], writes[2], tags[1])
	checkMatch(t, res.Res[1], writes[3], "")

	// Delete without params does nothing
	body = Body{"d", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	// Read back and validate all
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 5)
	checkMatch(t, res.Res[0], writes[0], "")
	checkMatch(t, res.Res[1], writes[1], tags[0])
	checkMatch(t, res.Res[2], writes[2], tags[1])
	checkMatch(t, res.Res[3], writes[3], "")
	checkMatch(t, res.Res[4], writes[4], tags[0])

	// Delete by idx
	body = Body{"d", usrname, sum, nil, "", "0"}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	// Read back and validate all
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 4)
	checkMatch(t, res.Res[0], writes[1], tags[0])
	checkMatch(t, res.Res[1], writes[2], tags[1])
	checkMatch(t, res.Res[2], writes[3], "")
	checkMatch(t, res.Res[3], writes[4], tags[0])

	// Write it back
	body = Body{"w", usrname, sum, writes[0], "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	// Delete by tag + idx
	body = Body{"d", usrname, sum, nil, tags[0], "4"}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	// Read back and validate
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 2)
	checkMatch(t, res.Res[0], writes[2], tags[1])
	checkMatch(t, res.Res[1], writes[3], "")

	// Delete by colliding tag + idx
	body = Body{"d", usrname, sum, nil, tags[1], "0"}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	// Read back and validate
	body = Body{"r", usrname, sum, nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err = parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 1)
	checkMatch(t, res.Res[0], writes[3], "")
}

func testNoPwd(t *testing.T) {
	usrname := "___GO___TEST___USR___"
	testCoreUsr(t, usrname, "")

	// Try and read with an unnecessary password
	body := Body{"r", usrname, fmt.Sprintf("%v", rand.Uint64()), nil, "", ""}
	b, _ := json.Marshal(body)
	request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response := httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 200)

	res, err := parseResponse(response)
	if err != nil {
		t.Error(err)
	}
	checkLen(t, res.Res, 1)

}

func testPwd(t *testing.T) {
	usrname := "___GO___TEST___USR___"
	sum := fmt.Sprintf("%v", rand.Uint64())
	testCoreUsr(t, usrname, sum)

	// Try and read with a wrong password
	body := Body{"r", usrname, fmt.Sprintf("%v", rand.Uint64()), nil, "", ""}
	b, _ := json.Marshal(body)
	request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response := httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 401)

	// Try and read without a password
	body = Body{"r", usrname, "", nil, "", ""}
	b, _ = json.Marshal(body)
	request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
	response = httptest.NewRecorder()
	UsrHandler(response, request)
	checkCode(t, response.Code, 401)
}

// Run the core tests with many users at once
func testStressManyUsers(t *testing.T, numUsers int) {
	// Generate random user info
	users := make([]string, numUsers)
	sums := make([]string, numUsers)
	for i := 0; i < numUsers; i++ {
		users[i] = fmt.Sprintf("___GO___TEST___USR___%v", i)
		if rand.Int()%2 == 1 {
			sums[i] = ""
		} else {
			sums[i] = fmt.Sprintf("%v", rand.Uint64())
		}
	}

	var wg sync.WaitGroup
	wg.Add(numUsers)
	for i := 0; i < numUsers; i++ {
		go func(userNum int) {
			testCoreUsr(t, users[userNum], sums[userNum])
			wg.Done()
		}(i)
	}
	wg.Wait()
}

func testUsrWriteReadDeleteOne(t *testing.T, usrname string, sum string, enc [][]int32, tags []string) {
	testCreateUsr(t, usrname, sum)
	// Write secrets one at a time, 50% with a random tag
	for _, e := range enc {
		var tag string
		if rand.Int()%2 == 1 {
			tag = ""
		} else {
			tag = tags[rand.Int()%len(tags)]
		}

		body := Body{"w", usrname, sum, e, tag, ""}
		b, _ := json.Marshal(body)
		request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
		response := httptest.NewRecorder()
		UsrHandler(response, request)
		checkCode(t, response.Code, 200)
	}

	numSecrets := 1 // Just some num > 0
	// While response len > 0, delete a secret at a random idx/tag and read them all back
	for numSecrets > 0 {
		var tag string
		if rand.Int()%2 == 1 {
			tag = ""
		} else {
			tag = tags[rand.Int()%len(tags)]
		}

		body := Body{"d", usrname, sum, nil, tag, fmt.Sprintf("%v", (rand.Int() % numSecrets))}
		b, _ := json.Marshal(body)
		request, _ := http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
		response := httptest.NewRecorder()
		UsrHandler(response, request)
		checkCode(t, response.Code, 200)

		body = Body{"r", usrname, sum, nil, "", ""}
		b, _ = json.Marshal(body)
		request, _ = http.NewRequest("POST", "/secrets/usr", bytes.NewBuffer(b))
		response = httptest.NewRecorder()
		UsrHandler(response, request)
		checkCode(t, response.Code, 200)

		res, err := parseResponse(response)
		if err != nil {
			t.Error(err)
		}
		numSecrets = len(res.Res)
	}
}

func testStressManySecrets(t *testing.T, numUsers int, numSecrets int) {
	// Generate random user info
	users := make([]string, numUsers)
	sums := make([]string, numUsers)
	for i := 0; i < numUsers; i++ {
		users[i] = fmt.Sprintf("___GO___TEST___USR___%v", i)
		if rand.Int()%2 == 1 {
			sums[i] = ""
		} else {
			sums[i] = fmt.Sprintf("%v", rand.Uint64())
		}
	}

	// For each user generate random secrets
	enc := make([][][]int32, numUsers)
	for i := 0; i < numUsers; i++ {
		enc[i] = make([][]int32, numSecrets)
		for j := 0; j < numSecrets; j++ {
			len := (rand.Int() % 63) + 1
			enc[i][j] = make([]int32, len)
			for k := 0; k < len; k++ {
				enc[i][j][k] = int32(rand.Uint32() % 255)
			}
		}
	}

	tags := make([]string, numSecrets)
	for i := 0; i < numSecrets; i++ {
		tags[i] = fmt.Sprintf("tag%v", i)
	}

	var wg sync.WaitGroup
	wg.Add(numUsers)
	for i := 0; i < numUsers; i++ {
		go func(userNum int) {
			testUsrWriteReadDeleteOne(t, users[userNum], sums[userNum], enc[userNum], tags)
			wg.Done()
		}(i)
	}
	wg.Wait()
}

func TestUsrHandler(t *testing.T) {
	args := os.Args
	if len(args) < 3 {
		panic("Missing argument: db authentication file")
	}
	uri := GetUri(args[2])

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

	start := time.Now()

	fmt.Println("Running testNoPwd")
	testNoPwd(t)
	fmt.Printf("Finished testNoPwd in %v seconds\n\n", time.Since(start))
	start = time.Now()

	fmt.Println("Running testPwd")
	testPwd(t)
	fmt.Printf("Finished testPwd in %v seconds\n\n", time.Since(start))
	start = time.Now()

	fmt.Println("Running testStressManyUsers")
	testStressManyUsers(t, 100)
	fmt.Printf("Finished testStressManyUsers in %v seconds\n\n", time.Since(start))
	start = time.Now()

	fmt.Println("Running testStressManySecrets")
	testStressManySecrets(t, 3, 200)
	fmt.Printf("Finished testStressManySecrets in %v seconds\n\n", time.Since(start))
}
