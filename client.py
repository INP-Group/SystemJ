# -*- encoding: utf-8 -*-

from src.server.client import ClientManager

def start():
    PORT = 9090
    client = ClientManager(PORT)

    with open('./media/test_channels', 'r') as f:
        lines = f.readlines()

    channels = []
    for line in lines:
        channels.append(tuple([x.strip() for x in line.strip().split(',')]))


    client.send("ADD_CH", *channels)


if __name__ == "__main__":
    start()