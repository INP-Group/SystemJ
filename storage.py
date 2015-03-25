# -*- encoding: utf-8 -*-

from src.storage.storage import Storage

def start():
    storage = Storage()
    storage.start()
    print("Server is stopped...")

if __name__ == '__main__':
    start()
