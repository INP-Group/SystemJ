# -*- encoding: utf-8 -*-

import sys

import project.settings
from project.settings import SIZEOF_UINT32
from src.server.control.consoleclient import ConsoleClient
from src.base.channellfactory import ChannelFactory

if not project.settings.DEPLOY:
    import DLFCN

    old_dlopen_flags = sys.getdlopenflags()
    sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
    from PyQt4.QtCore import *
    from PyQt4.QtGui import *
    from PyQt4.QtNetwork import *

    sys.setdlopenflags(old_dlopen_flags)
else:
    from PyQt4.QtCore import *
    from PyQt4.QtGui import *
    from PyQt4.QtNetwork import *


class Manager(ConsoleClient):

    def __init__(self, argv, host, port):
        client_name = 'Test_manager'
        super(Manager, self).__init__(argv, host, port, client_name)

        self.all_channels = []

        self.workers = {}

        self._add_command('CHL_ADD', self._command_channel_add)
        self._add_command('CHL_LIST', self._command_channel_list)
        self._add_command('CHL_DEL', self._command_channel_add) #todo
        self._add_command('WORKER_ADD', self._command_worker_add)
        self._add_command('WORKER_LIST', self._command_worker_list)
        self._add_command('WORKER_DEL', self._command_worker_add) #todo
        self._add_command('HI', self._command_set_type)

    def _command_set_type(self, command, message):
        assert command
        assert message
        self.send_message('SET_TYPE', 'manager')

    def _command_channel_list(self, command, message):
        assert command
        assert message
        assert self.channels
        self.send_message('CHL_LIST', str(self.channels))

    def _command_channel_add(self, command, message):
        assert command
        assert message
        assert self.workers
        self._log('Add channel in random worker')
        # todo
        # параметры канала хранить где?

        if str(message) not in self.all_channels or True:
            self.all_channels.append(str(message))

            #todo надо вставлять в конкретный
            worker = self.workers.values()[0]
            assert isinstance(worker, DaemonWorker)
            worker.add_channel(chanName=str(message))

            self._log('Added channel')
        else:
            self._log('Channel already exist')

    def _command_worker_add(self, command, message):
        assert command
        assert message
        self._log("Add new worker")
        if str(message) not in self.workers:
            self.workers[str(message)] = DaemonWorker(name=str(message))
            self._log('Added worker')
        else:
            self._log('Worker already exist')

    def _command_worker_list(self, command, message):
        assert command
        assert message
        assert self.workers
        self.send_message('WORKER_LIST', str(self.workers.keys()))


class DaemonWorker(QThread):

    """10к сокетов не получается создать.

    Надо сигналы одной группы собирать с помощью этого воркера Затем
    слать одно сообщение в zeromq (ну или хотя бы по M сообщений
    собирать в одно)

    """

    def __init__(self, parent=None, name='Daemon1'):

        QThread.__init__(self, parent)
        self.name = name
        self.channels = []

    def __del__(self):
        self.wait()

    def get_channels(self):
        return self.channels

    def get_name(self):
        return self.name

    def add_channel(self, type='ScalarChannel',
                  chanName='linthermcan.ThermosM.in0'):

        # todo
        type = 'ScalarChannel'

        channel = ''

        if type == 'ScalarChannel':
            channel = ChannelFactory.factory(
                type, chanName, '%s - %s' %
                (self.name, len(
                    self.channels)))

        if type == 'NTimeChannel':
            channel = ChannelFactory.factory(
                type, chanName, '%s - %s' %
                (self.name, len(
                    self.channels)))
            channel.set_property('timedelta', 5.0)

        if type == 'DeltaChannel':
            channel = ChannelFactory.factory(
                type, chanName, '%s - %s' %
                (self.name, len(
                    self.channels)))
            channel.set_property('delta', .0005)

        self.channels.append(channel)

