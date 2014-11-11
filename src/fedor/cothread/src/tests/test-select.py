#!/usr/bin/env python2.6

# Testing coselect

from __future__ import print_function

import threading
import time

from cothread import *


def ThreadTicker(queue):
    while True:
        print('tick', )
        queue.Signal('tick')
        time.sleep(1)


def Listener(queue, n):
    while True:
        x = queue.Wait()
        print('listener', n, x)


def Ticker(n):
    while True:
        print('ticker', n)
        Sleep(n)


queue = ThreadedEventQueue()

ticker = threading.Thread(target=ThreadTicker, args=(queue,))
ticker.setDaemon(True)
ticker.start()

Spawn(Listener, queue, 1)
Spawn(Listener, queue, 2)
Spawn(Ticker, 0.5)

WaitForQuit()
