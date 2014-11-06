# -*- encoding: utf-8 -*-


from src.base.daemon import Daemon
import uuid
import os

class BaseDaemonChannel(Daemon):

    def __init__(self):
        super(BaseDaemonChannel, self).__init__(self._generate_random_filepath())

    def _generate_random_filepath(self):
        PID_FOLDER = '/tmp/pids/'

        if not os.path.exists(PID_FOLDER):
            os.makedirs(PID_FOLDER)

        return os.path.join(PID_FOLDER, str(uuid.uuid4()) + ".pid")

    def run(self):

        while True:
            a = 1