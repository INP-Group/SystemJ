# -*- encoding: utf-8 -*-

import sys

import project.settings
from src.server.control.base.baseserver import BaseServer

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

from project.settings import SIZEOF_UINT32


class ControlServer(BaseServer):
    def __init__(self, argv, host, port):
        super(ControlServer, self).__init__(argv, host, port)

        self._add_command("MANAGER_LIST", self._command_users)
        self._add_command("SET_TYPE", self._command_set_type)
        self._add_command("CHANNEL", self._command_channel)

    def _command_set_type(self, client, command, message):
        assert self.users
        self.users[client]['type'] = message
        self.send_message(client, "SET")

    def _command_channel(self, client, command, message):
        self._log("Add channel")
        # todo
        # Добавить возможность добавлять в определенный менеджер

        for client, info in self.users.items():
            if info.get('type') == 'manager':
                self.send_message(client, "CHANNEL_ADD", "")
        self._log("Added channel")

