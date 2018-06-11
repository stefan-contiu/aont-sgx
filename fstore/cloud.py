from flask import Flask
from flask import request
import redis
import hashlib

REDIS_METADATA_SERVER_NAME = "127.0.0.1"
REDIS_METADATA_SERVER_PORT = "6379"

STORAGES = [
    "/media/stefan/Windows/PHD/share1/",
    "/media/stefan/Windows/PHD/share2/",
    "/media/stefan/Windows/PHD/share3/",
    "/media/stefan/Windows/PHD/share4/",
    "/media/stefan/Windows/PHD/share5/"]

app = Flask(__name__)
r = redis.StrictRedis(host=REDIS_METADATA_SERVER_NAME, port=REDIS_METADATA_SERVER_PORT, db=0)

@app.route('/')
def index():
    return "Hello, World!"

@app.route("/write_metadata/<path:filename>", methods=['POST'])
def post_meta(filename):
    data = request.get_data()
    print("I will save meta %s of size %d" % (filename, len(data)))
    r.set(filename, data)
    return ""

@app.route("/write_block/<path:filename>", methods=['POST'])
def post_block(filename):
    data = request.get_data()
    print("I will save BLOCK %s of size %d" % (filename, len(data)))

    # strip the block index
    k = filename.rfind(".")
    short_file_name = filename[:k].encode('utf-8')
    # uniform hash the file name over the storages count
    N = len(STORAGES)
    h = hashlib.sha1(short_file_name)
    v = int(h.hexdigest(), 16)
    i = v % N

    # save to drive
    storage_file_name = STORAGES[i] + filename
    output_file = open(storage_file_name, "wb")
    output_file.write(data)
    output_file.close()

    # save to redis
    r.set(filename, storage_file_name)

    return ""

@app.route("/read_metadata/<path:filename>", methods=['GET'])
def get_meta(filename):
    data = r.get(filename)
    return data

@app.route("/read_block/<path:filename>", methods=['GET'])
def get_block(filename):
    storage_file_name = r.get(filename)
    input_file = open(storage_file_name, "rb")
    data = input_file.read()
    input_file.close()
    return data

if __name__ == '__main__':
    app.run(debug=True)
