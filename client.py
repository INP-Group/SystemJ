# -*- encoding: utf-8 -*-

# from src.server.first.client import ClientManager
# from project.settings import SERVER_PORT
import socket
import sys
from project.settings import SERVER_PORT, SERVER_HOST
from src.server.make.clientmanager import ClientManager


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




    client = ClientManager(SERVER_HOST, SERVER_PORT)

    client.send_online()

    client.send_offline()

    # data = "gfdokgosdfg sdfg jdsfngoj sdnfgio ndfg dfgsdf"
    # for x in xrange(0, 5):
    #     client.send(data)




if __name__ == "__main__":
    start()


