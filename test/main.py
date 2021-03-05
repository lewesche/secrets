import requests
import pytest
from pymongo import MongoClient


@pytest.fixture
def db():
    db_url = 'mongodb+srv://lewesche:1234@cluster0.e6ckn.mongodb.net/myFirstDatabase?retryWrites=true&w=majority'
    client = MongoClient(db_url)
    db = client.secrets.users
    return db

def pass_check(res):
    assert 'success' in res
    assert res['success'] == 'true'
    assert 'e' not in res
    assert 'res' in res


def fail_check(res):
    assert 'success' in res
    assert res['success'] == 'false'
    assert 'e' in res

def test_no_pwd(db):
    db.delete_one({'usr':'pytestusr'})
 
    usr_info = {'usr':'pytestusr'}
    core_tests(usr_info)

    # Try reading back with unused password
    url = 'https://lewesche.com/secrets/usr'
    body = usr_info.copy()
    body['action'] = 'r'
    body['pwd'] = '1234'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 1
    assert 'enc' in res['res'][0]

def test_pwd(db):
    db.delete_one({'usr':'pytestusr_password'})
 
    usr_info = {'usr':'pytestusr_password', 'pwd':'1234'}
    core_tests(usr_info)

    # Try reading back with wrong password
    url = 'https://lewesche.com/secrets/usr'
    body = usr_info.copy()
    body['action'] = 'r'
    body['pwd'] = '123'
    res = requests.post(url, json = body).json()
    fail_check(res)

    # Try reading back without a password
    body = {'usr':'pytestusr_password'}
    body['action'] = 'r'
    res = requests.post(url, json = body).json()
    fail_check(res)


def core_tests(usr_info):
    # Create testusr from api
    url = 'https://lewesche.com/secrets/new'
    body = usr_info.copy()
    body['action'] = 'c'
    res = requests.post(url, json = body).json()
    pass_check(res)

    # Try reading - should be empty
    url = 'https://lewesche.com/secrets/usr'
    body = usr_info.copy()
    body['action'] = 'r'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 0
    assert 'e' not in res

    # Try write without data
    body = usr_info.copy()
    body['action'] = 'w'

    res = requests.post(url, json = body).json()
    fail_check(res)


    write = ['hello world', 'mellow world', 'yellow mellow', 'yellow world', 'dlrow olleh']

    # Try write with data
    body = {'action':'w', 'data':write[0], 'usr':'pytestusr'}
    body = usr_info.copy()
    body['action'] = 'w'
    body['data'] = write[0]

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Try reading back
    body = usr_info.copy()
    body['action'] = 'r'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 1
    assert 'enc' in res['res'][0]
    assert write[0] in res['res'][0]['enc']
    assert 'tag' not in res['res'][0]

    # Try reading back with unused data
    body = usr_info.copy()
    body['action'] = 'r'
    body['data'] = write[0]

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 1
    assert 'enc' in res['res'][0]
    assert write[0] in res['res'][0]['enc']
    assert 'tag' not in res['res'][0]

    # Try reading back with unused password
    #body = {'action':'r', 'pwd':'1234', 'usr':'pytestusr'}
    #body = usr_info
    #body['action'] = 'c'

    #res = requests.post(url, json = body).json()
    #pass_check(res)
    #assert len(res['res']) == 1
    #assert 'enc' in res['res'][0]
    #assert write[0] in res['res'][0]['enc']
    #assert 'tag' not in res['res'][0]

    tag = ['1', '2']

    # Try write with a tag
    body = usr_info.copy()
    body['action'] = 'w'
    body['data'] = write[1]
    body['tag'] = tag[0]

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Try reading both entires
    body = usr_info.copy()
    body['action'] = 'r'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 2
    
    assert 'enc' in res['res'][0]
    assert write[0] in res['res'][0]['enc']
    assert 'tag' not in res['res'][0]

    assert 'enc' in res['res'][1]
    assert write[1] in res['res'][1]['enc']
    assert 'tag' in res['res'][1]
    assert tag[0] in res['res'][1]['tag']

    # Write some more entires, two with tags and one without
    body = usr_info.copy()
    body['action'] = 'w'
    body['data'] = write[2]
    body['tag'] = tag[1]

    res = requests.post(url, json = body).json()
    pass_check(res)

    body = usr_info.copy()
    body['action'] = 'w'
    body['data'] = write[3]

    res = requests.post(url, json = body).json()
    pass_check(res)

    body = usr_info.copy()
    body['action'] = 'w'
    body['data'] = write[4]
    body['tag'] = tag[0]

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Read and validate all
    body = usr_info.copy()
    body['action'] = 'r'
    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 5
    
    assert 'enc' in res['res'][0]
    assert write[0] in res['res'][0]['enc']
    assert 'tag' not in res['res'][0]

    assert 'enc' in res['res'][1]
    assert write[1] in res['res'][1]['enc']
    assert 'tag' in res['res'][1]
    assert tag[0] in res['res'][1]['tag']

    assert 'enc' in res['res'][2]
    assert write[2] in res['res'][2]['enc']
    assert 'tag' in res['res'][2]
    assert tag[1] in res['res'][2]['tag']

    assert 'enc' in res['res'][3]
    assert write[3] in res['res'][3]['enc']
    assert 'tag' not in res['res'][3]

    assert 'enc' in res['res'][4]
    assert write[4] in res['res'][4]['enc']
    assert 'tag' in res['res'][4]
    assert tag[0] in res['res'][4]['tag']

    # Read by idx
    body = usr_info.copy()
    body['action'] = 'r'
    body['idx'] = '3'

    res = requests.post(url, json = body).json()
    pass_check(res) 
    assert len(res['res']) == 1
    assert 'enc' in res['res'][0]
    assert write[3] in res['res'][0]['enc']
    assert 'tag' not in res['res'][0]

    # Read by tag
    body = usr_info.copy()
    body['action'] = 'r'
    body['tag']=tag[1]

    res = requests.post(url, json = body).json()
    pass_check(res) 
    assert len(res['res']) == 1
    assert 'enc' in res['res'][0]
    assert write[2] in res['res'][0]['enc']
    assert 'tag' in res['res'][0]
    assert tag[1] in res['res'][0]['tag']

    # Read by tag + idx
    body = usr_info.copy()
    body['action'] = 'r'
    body['tag']=tag[1]
    body['idx'] = '3'

    res = requests.post(url, json = body).json()
    pass_check(res) 
    assert len(res['res']) == 2

    assert 'enc' in res['res'][0]
    assert write[2] in res['res'][0]['enc']
    assert 'tag' in res['res'][0]
    assert tag[1] in res['res'][0]['tag']

    assert 'enc' in res['res'][1]
    assert write[3] in res['res'][1]['enc']
    assert 'tag' not in res['res'][1]

    # Delete without params does nothing
    body = usr_info.copy()
    body['action'] = 'd'

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Delete by idx
    body = usr_info.copy()
    body['action'] = 'd'
    body['idx'] = '0'

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Read and validate all
    body = usr_info.copy()
    body['action'] = 'r'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 4
    
    assert 'enc' in res['res'][0]
    assert write[1] in res['res'][0]['enc']
    assert 'tag' in res['res'][0]
    assert tag[0] in res['res'][0]['tag']

    assert 'enc' in res['res'][1]
    assert write[2] in res['res'][1]['enc']
    assert 'tag' in res['res'][1]
    assert tag[1] in res['res'][1]['tag']

    assert 'enc' in res['res'][2]
    assert write[3] in res['res'][2]['enc']
    assert 'tag' not in res['res'][2]

    assert 'enc' in res['res'][3]
    assert write[4] in res['res'][3]['enc']
    assert 'tag' in res['res'][3]
    assert tag[0] in res['res'][3]['tag']

    # Write it back
    body = usr_info.copy()
    body['action'] = 'w'
    body['data'] = write[0]

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Delete by tag and idx
    body = usr_info.copy()
    body['action'] = 'd'
    body['tag']=tag[0]
    body['idx'] = '4'

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Read and validate all
    body = usr_info.copy()
    body['action'] = 'r'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 2
    
    assert 'enc' in res['res'][0]
    assert write[2] in res['res'][0]['enc']
    assert 'tag' in res['res'][0]
    assert tag[1] in res['res'][0]['tag']

    assert 'enc' in res['res'][1]
    assert write[3] in res['res'][1]['enc']
    assert 'tag' not in res['res'][1]

    # Delete by colliding tag and idx
    body = usr_info.copy()
    body['action'] = 'd'
    body['tag']=tag[1]
    body['idx'] = '0'

    res = requests.post(url, json = body).json()
    pass_check(res)

    # Read and validate
    body = usr_info.copy()
    body['action'] = 'r'

    res = requests.post(url, json = body).json()
    pass_check(res)
    assert len(res['res']) == 1
    
    assert 'enc' in res['res'][0]
    assert write[3] in res['res'][0]['enc']
    assert 'tag' not in res['res'][0]

#if __name__ == "__main__":
    #test_pwd()
    #test_no_pwd()
