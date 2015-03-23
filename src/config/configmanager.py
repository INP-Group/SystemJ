# -*- encoding: utf-8 -*-

import configparser
from src.base.singleton import Singleton


class ConfigManager(Singleton):

    def __init__(self):
        self.config = configparser.ConfigParser()
        #todo
        self.default_path = '/home/warmonger/Dropbox/Study/Diploma/Diploma/resources/config/config.ini'

    def get_config(self):
        return self.config

    def save(self, filepath=None):
        if filepath is None:
            filepath = self.default_path
        with open(filepath, 'w') as configfile:
            self.config.write(configfile)
