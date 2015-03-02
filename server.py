# -*- encoding: utf-8 -*-

#
# import time
# from bottle import route, run
#
# @route('/')
# def index():
#     return {'status':'online', 'servertime':time.time()}
#
# run(host='localhost', port=8080)
#
#


from src.server.first.server import ServerManager

def start():
    PORT = 9090
    server = ServerManager(PORT)
    server.run()


if __name__ == "__main__":
    start()