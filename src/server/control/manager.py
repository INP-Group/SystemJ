# -*- encoding: utf-8 -*-

import sys
import project.settings

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
from project.settings import SERVER_PORT, SERVER_HOST, SIZEOF_UINT32
from src.channels.simplechannel import SimpleChannel
from src.server.control.consoleclient import ConsoleClient


class Manager(ConsoleClient):
    def __init__(self, argv, host, port):
        client_name = "Test_manager"
        super(Manager, self).__init__(argv, host, port, client_name)

        self.channels = {}

        self._add_command("CHANNEL_ADD", self._command_channel_add)
        self._add_command("CHANNEL_LIST", self._command_channel_add)
        self._add_command("CHANNEL_REMOVE", self._command_channel_add)
        self._add_command("HI", self._command_set_type)

    def _command_set_type(self, command, message):
        print("gfkmdngjdsnfgosdnfhojsdnfhodsf")
        self.send_message("SET_TYPE", 'manager')

    def _command_channel_list(self, command, message):
        self.send_message("CHANNEL_LIST", str(self.channels.keys()))

    def _command_channel_add(self, command, message):
        self._log("Add channel")
        # todo
        # параметры канала хранить где?
        name = "linthermcan.ThermosM.in0"
        if name not in self.channels:
            self.channels[name] = SimpleChannel(name)
            self._log("Added channel")
        else:
            self._log("Channel already exist")

