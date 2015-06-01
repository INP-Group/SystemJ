# -*- encoding: utf-8 -*-

import sys

import project.settings
import zmq
from project.logs import log_debug
from project.settings import ZEROMQ_HOST, ZEROMQ_PORT
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
        if '___' in message:
            channel_name, properties = [x.strip() for x in message.split('___')]
            properties = eval(properties)
        else:
            channel_name = message.strip()
            properties = {}

        assert isinstance(properties, dict)
        assert channel_name
        assert isinstance(channel_name, str)

        if channel_name not in self.all_channels or True:
            self.all_channels.append(message)

            # todo
            # равномерно раскидываем каналы по воркерам
            min_channels = sys.maxsize
            min_worker = None
            for worker in self.workers.values():
                if worker.get_len_channels() < min_channels:
                    min_worker = worker
                    min_channels = worker.get_len_channels()

            assert isinstance(min_worker, DaemonWorker)
            min_worker.add_channel(chanName=channel_name,
                                   properties=properties)

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
        self.startTimer(1000)  # every second
        self.channels_data = []

        # todo должна быть возможность подмены этих параметров динамически
        self.host = ZEROMQ_HOST
        self.port = ZEROMQ_PORT
        self.attempt = 0
        self.max_attempt = 10

    def timerEvent(self, event):
        """Каждый период времени отправляем пачку в zeromq.

        :param event:
        :return:

        """
        if self.channels_data:
            data_for_storage = {}
            for data in self.channels_data:
                if not data.get('name') in data_for_storage:
                    data_for_storage[data.get('name')] = []
                data_for_storage[data.get('name')].append(
                    {'time': data.get('time'),
                     'value': data.get('value')})

            self.send_data(data_for_storage)
        self.channels_data = []

    def get_len_channels(self):
        return len(self.channels)

    def __del__(self):
        self.wait()

    def get_channels(self):
        return self.channels

    def get_name(self):
        return self.name

    def store_data(self, *args):
        self.channels_data.append(args[1])

    def send_data(self, json_data):
        # init zeromqsocket
        if self.attempt >= self.max_attempt:
            self.store_data(json_data)
            return

        context = zmq.Context()
        sock = context.socket(zmq.REQ)
        sock.connect('tcp://%s:%s' % (self.host, self.port))

        sock.send_json(json_data)
        answer = sock.recv()
        if answer.strip() == 'Saved':
            sock.close()
            self.attempt = 0
            return
        else:
            sock.close()
            self.attempt += 1
            self.send_message(json_data)

    def add_channel(self,
                    monitor_type='ScalarMonitor',
                    chanName='linthermcan.ThermosM.in0',
                    properties=None):

        # todo
        monitor_type = properties.get('type', 'SimpleMonitor')
        frequency = properties.get('frequency', 100)

        channel = MonitorFactory.factory(monitor_type, chanName,
                                         '%s - %s' % (self.name,
                                                      len(self.channels)),
                                         frequency=frequency)
        if monitor_type == 'NTimeMonitor':
            channel.set_property('timedelta', 5.0)

        if monitor_type == 'DeltaMonitor':
            channel.set_property('delta', .0005)

        assert channel is not None

        if isinstance(properties, dict) and properties:
            for key, value in properties.items():
                channel.set_property(key, value)
        channel.valueToStorage.connect(self.store_data)
        self.channels.append(channel)
