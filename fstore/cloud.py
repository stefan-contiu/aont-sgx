from flask import Flask
from flask import request
import redis
import hashlib
import paramiko
import binascii
import os

#data = 'This is arbitrary data\n'.encode('ascii')
#put_file('192.168.1.116', 'nuc', 'msstream', '/hdd', 'file.bin', data)


REDIS_METADATA_SERVER_NAME = "192.168.1.112"
REDIS_METADATA_SERVER_PORT = "6379"

HDRIVES = {}
HDRIVES[0] = ["192.168.1.118", "nuc", "msstream", "/hdd", "nuc11"]       # NUC11
HDRIVES[1] = ["192.168.1.116", "nuc", "msstream", "/hdd", "nuc12"]       # NUC12
HDRIVES[2] = ["192.168.1.114", "nuc", "msstream", "/hdd", "nuc13"]       # NUC13
HDRIVES[3] = ["192.168.1.106", "nuc", "msstream", "/hdd", "nuc14"]       # NUC14
HDRIVES[4] = ["192.168.1.102", "nuc17", "msstream", "/hdd", "nuc17"]     # NUC17
HDRIVES[5] = ["192.168.1.103", "nuc19", "msstream", "/hdd", "nuc19"]     # NUC19

app = Flask(__name__)
r = redis.StrictRedis(host=REDIS_METADATA_SERVER_NAME, port=REDIS_METADATA_SERVER_PORT, db=0)

def put_file(machinename, username, password, dirname, filename, data):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(machinename, username=username, password=password)
    sftp = ssh.open_sftp()
    f = sftp.open(dirname + '/' + filename, 'w')
    f.write(data)
    f.close()
    ssh.close()

def clear_share(m, u, p, d):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(m, username=u, password=p)
    sftp = ssh.open_sftp()

    filesInRemoteArtifacts = sftp.listdir(path=d)
    for file in filesInRemoteArtifacts:
        sftp.remove(os.path.join(d, file))

    sftp.close()
    ssh.close()

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
    N = len(HDRIVES.keys())
    v = binascii.crc32(short_file_name)
    i = v % N

    # save to drive
    put_file(HDRIVES[i][0], HDRIVES[i][1], HDRIVES[i][2], HDRIVES[i][3], filename, data)

    # save to redis the worker name
    r.set(short_file_name, HDRIVES[i][4])

    return ""

@app.route("/read_metadata/<path:filename>", methods=['GET'])
def get_meta(filename):
    data = r.get(filename)
    return data

@app.route("/read_block/<path:filename>", methods=['GET'])
def get_block(filename):
    k = filename.rfind(".")
    short_file_name = filename[:k]
    worker_name = r.get(short_file_name)

    # based on worker name get IP address and file content
    # TODO : go to
    storage_file_name = r.get(filename)
    input_file = open(storage_file_name, "rb")
    data = input_file.read()
    input_file.close()
    return data

if __name__ == '__main__':

    print("Cleaning shares ...")
    for k in HDRIVES:
        print("Clearning", HDRIVES[k][0])
        clear_share(HDRIVES[k][0], HDRIVES[k][1], HDRIVES[k][2], HDRIVES[k][3])

    app.run(debug=True, host='0.0.0.0')
