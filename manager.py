# -*- encoding: utf-8 -*-

import os
import sys

from src.channels.daemon import Daemon

import argparse
from src.channels.scalarchannel import ScalarChannel


class DaemonManager(Daemon):

    def __init__(self):
        super(DaemonManager, self).__init__(self._generate_random_filepath(), stdin='/dev/stdin', stdout='/dev/stdout', stderr='/dev/stderr')

    def _generate_random_filepath(self):
        # PID_FOLDER = '/tmp/pids/'
        PID_FOLDER = os.path.abspath('./')
        manage_pid_name = 'daemon_manage'

        if not os.path.exists(PID_FOLDER):
            os.makedirs(PID_FOLDER)

        return os.path.join(PID_FOLDER, manage_pid_name + ".pid")

    def run(self):
        while True:
            pass

    def startD(self, name):

        # Special QT import, needs to make qt libs visible for Cdrlib
        import DLFCN
        old_dlopen_flags = sys.getdlopenflags( )
        sys.setdlopenflags( old_dlopen_flags | DLFCN.RTLD_GLOBAL )
        from PyQt4 import QtCore, QtGui
        sys.setdlopenflags( old_dlopen_flags )  # set default value
        scal = ScalarChannel("linthermcan.ThermosM.in0")


        print "Create new daemon: ", name

    def stopD(self, name):
        print "Stop daemon: ", name

    def addChD(self, daemon_name, chan_name):
        print "Add new channel ", chan_name, " in to", daemon_name

    def remChD(self, daemon_name, chan_name):
        print "Remove  channel ", chan_name, " in to", daemon_name


'''
Надо складывать:

Для демона: 

  - Название
  - Путь до pid'а
  - Когда был запущен
  - Когда последний раз модифицировался
  - Количество каналов

Для канала:

  - Название
  - <короткое название>
  - Когда был запущен
  - Тип канала (какой класс калана заводить)
  - <параметры>


ПРОБЛЕМА:
при каждом обращании к демону надо его пересоздать, а значит и каналы надо будет перезапускать


export CHLCLIENTS_PATH=/home/warmonger/Dropbox/Study/Diploma/Diploma_cx/work/qult/chlclients; export CX_TRANSLATE_LINAC1_34=:34;


'''


def gDO(pin_name='pid'):
    return DaemonManager()

def process_input_data(**kwargs):
    if kwargs.get("start", False):
        ob = gDO()
        ob.start()
    if kwargs.get("stop", False):
        ob = gDO()
        ob.stop()
    if kwargs.get("startdaemon", ""):
        ob = gDO()
        ob.startD(kwargs.get("startdaemon"))
    if kwargs.get("stopdaemon", ""):
        ob = gDO()
        ob.startD(kwargs.get("stopdaemon"))
    if kwargs.get("addchannel", []):
        value = kwargs.get("removechannel")
        ob = gDO()
        ob.addChD(value[0], value[1])
    if kwargs.get("removechannel", []):
        value = kwargs.get("removechannel")
        ob = gDO()
        ob.remChD(value[0], value[1])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--start', help='Starts %(prog)s daemon', action='store_true')
    parser.add_argument('-q', '--stop', help='Stop %(prog)s daemon', action='store_true')
    parser.add_argument('-c', '--startdaemon', help='Start new daemon')
    parser.add_argument('-d', '--stopdaemon', help='Stop daemon')
    parser.add_argument('-a', '--addchannel', help='Add new channel to daemon', default=[], action='store', nargs='*')
    parser.add_argument('-r', '--removechannel', help='Remove channel from daemon daemon', default=[], action='store', nargs='*')

    args = parser.parse_args()

    process_input_data(**vars(args))