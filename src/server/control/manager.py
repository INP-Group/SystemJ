# -*- encoding: utf-8 -*-

import sys

import project.settings
from project.logs import log_debug
from project.settings import SIZEOF_UINT32
from src.base.monitorfactory import MonitorFactory
from src.server.control.consoleclient import ConsoleClient

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

    def __init__(self, host, port):
        client_name = 'Test_manager'
        super(Manager, self).__init__(host, port, client_name)

        self.all_channels = []

        self.workers = {}

        self._add_command('CHL_ADD', self._command_channel_add)
        self._add_command('CHL_LIST', self._command_channel_list)
        self._add_command('CHL_DEL', self._command_channel_add)  # todo
        self._add_command('WORKER_ADD', self._command_worker_add)
        self._add_command('WORKER_LIST', self._command_worker_list)
        self._add_command('WORKER_DEL', self._command_worker_add)  # todo
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
        log_debug('Add channel in random worker')
        # todo
        # параметры канала хранить где?
        message = str(message)
        channel_name, properties = [x.strip() for x in message.split('___')]
        properties = eval(properties)

        if channel_name not in self.all_channels or True:
            self.all_channels.append(message)

            # todo надо вставлять в конкретный
            worker = self.workers.values()[0]
            assert isinstance(worker, DaemonWorker)
            worker.add_channel(chanName=channel_name, properties=properties)

            log_debug('Added channel')
        else:
            log_debug('Monitor already exist')

    def _command_worker_add(self, command, message):
        assert command
        assert message
        log_debug('Add new worker')
        if str(message) not in self.workers:
            self.workers[str(message)] = DaemonWorker(name=str(message))
            log_debug('Added worker')
        else:
            log_debug('Worker already exist')

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

    def add_channel(self, monitor_type='ScalarMonitor',
                    chanName='linthermcan.ThermosM.in0',
                    properties=None):

        # todo
        monitor_type = 'ScalarMonitor'
        if 'type' in properties:
            monitor_type = properties.get('type')

        channel = MonitorFactory.factory(monitor_type, chanName, '%s - %s' %
                                         (self.name, len(
                                             self.channels)))
        if monitor_type == 'NTimeMonitor':
            channel.set_property('timedelta', 5.0)

        if monitor_type == 'DeltaMonitor':
            channel.set_property('delta', .0005)

        assert channel is not None

        if isinstance(properties, dict) and properties:
            for key, value in properties.items():
                channel.set_property(key, value)

        self.channels.append(channel)
