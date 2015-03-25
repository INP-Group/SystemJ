# -*- encoding: utf-8 -*-

# from src.server.first.client import ClientManager
# from project.settings import SERVER_PORT
import socket
import sys
from project.settings import SERVER_PORT, SERVER_HOST
from src.server.make.client import Client


def start():
    # client = ClientManager(SERVER_PORT)
    #
    # with open('./media/test_channels', 'r') as f:
    #     lines = f.readlines()
    #
    # channels = []
    # for line in lines:
    #     channels.append(tuple([x.strip() for x in line.strip().split(',')]))
    #
    # client.send("ADD_CH", *channels)




    client = Client(SERVER_HOST, SERVER_PORT)

    data = "gfdokgosdfg sdfg jdsfngoj sdnfgio ndfg dfgsdf"
    for x in xrange(0, 5):
        client.send(data)




if __name__ == "__main__":
    start()


