from __future__ import print_function

import socket

import cothread

cothread.socket_hook()

SERVER = 'localhost'
PORT = 8888

s = socket.create_connection((SERVER, PORT))
print('sending', s.send('bogus\n'))
print('recving', repr(s.recv(1024)))

while True:
    input = raw_input('> ')
    s.sendall(input)
    print(s.recv(1024))
