# -*- encoding: utf-8 -*-

# from src.server.first.client import ClientManager
# from project.settings import SERVER_PORT
import socket
import sys
from project.settings import SERVER_PORT, SERVER_HOST, MANAGER_TEST_PORT, \
    MANAGER_TEST_HOST
from src.server.make.clientmanager import ClientManager
from src.server.make.yetclient import YetClient


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

    #



    yet_client = YetClient(host=SERVER_HOST, port=SERVER_PORT)

    yet_client.send_hi()
    yet_client.get_manager_cnt()
    yet_client.get_manager_list()
    # yet_client.set_channels([])

    # client.send_offline()

    # data = "gfdokgosdfg sdfg jdsfngoj sdnfgio ndfg dfgsdf"
    # for x in xrange(0, 5):
    #     client.send(data)




if __name__ == "__main__":
    start()


