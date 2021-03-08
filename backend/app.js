const express = require('express')
const { MongoClient } = require("mongodb");

const app = express();
app.use(express.json())

const uri = 'mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/secrets?retryWrites=true&w=majority'
const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true });
// Leave connection open for thread pooling
client.connect();

function db_read(obj, res) {
	let tag = obj.tag
	let idx = obj.idx

	let secrets = res.secrets
	return secrets.filter( (s, i) => {
		if(!tag && !idx) {
			return true
		}
		if(tag && ('tag' in s) && s.tag == tag) {
			return true;
		}
		if(idx && idx==i) {
			return true
		}
	})
}

async function db_write(obj) {
	const coll = client.db('secrets').collection('users')
	let filter = {'usr': obj.usr}

	let res
	if('tag' in obj) {
		let push = {"$push": {"secrets": { "tag": obj.tag, "enc": obj.data } }}
		res = await coll.updateOne(filter, push)
	} else {
		let push = {"$push": {"secrets": { "enc": obj.data } }}
		res = await coll.updateOne(filter, push)
	}

	let msg = "Modified count = " + res.modifiedCount
	return msg
}

async function db_delete(obj) {
	const coll = client.db('secrets').collection('users')
	let filter = {'usr': obj.usr}
	let sum = 0

	// delete by index first
	if('idx' in obj) {
		let arr_idx = 'secrets.' + obj.idx
		let unset = {"$unset": { [arr_idx]: 0 } }
		res = await coll.updateOne(filter, unset)
		sum += res.modifiedCount

		let pull = {"$pull": {"secrets": null} }
		res = await coll.updateOne(filter, pull)
	} 
	
	if('tag' in obj) {
		let pull = {"$pull": { 'secrets': {'tag':obj.tag} } }
		res = await coll.updateOne(filter, pull)
		sum += res.modifiedCount
	} 

	let msg = "Modified count = " + sum
	return msg
}

async function db_create(obj) {
	const coll = client.db('secrets').collection('users')
	
	let newUser
	if('sum' in obj) {
		newUser = {'usr':obj.usr, 'sum':obj.sum, 'secrets':[]}
	} else {
		newUser = {'usr':obj.usr, 'secrets':[]}
	}
	await coll.insertOne(newUser)
	return "User created"
}

async function authenticate(obj) {
    const coll = client.db('secrets').collection('users')
	let res = await coll.findOne({'usr': obj.usr})

	if(!res) {
		if(obj.action != 'c') {
			throw "User not found"
		}
		return
	} else {
		if(obj.action == 'c') {
			throw "User already exists"
		}
	}

	if ('sum' in res) {
		if (!('sum' in obj)) {
			throw 'User expects a password, none provided'
		}
		if (obj.sum != res.sum)
			throw 'Passwords do not match'
	}
	return res 
}

function check_request(obj) {
	if (!('action' in obj))
		throw 'Missing action!'
	if (!('usr' in obj))
		throw 'Missing user!'
	if ((obj.action == "w") && !('data' in obj))
		throw 'Missing data to write!'
}

function fail(e) {
	console.log(e)
	return {'success':'false', 'e':e}
}

function success(res) {
	return {'success':'true', 'res':res}
}

app.post('/secrets/usr', async (req, res) => {
	try {
		check_request(req.body)
		let action = req.body.action

		let response = await authenticate(req.body)

		if (action == 'r') {
			response = await db_read(req.body, response)
		} else if (action == 'w') {
			response = await db_write(req.body)
		} else if (action =='d') {
			response = await db_delete(req.body)
		} else if (action =='c') {
			response = await db_create(req.body)
		} else {
			throw 'Unknown action!'
		}
		res.json(success(response))
	} catch (e) {
		res.json(fail(e))
	}
})

const PORT = 8000
app.listen(PORT, () => console.log('Running: port = ', PORT))

