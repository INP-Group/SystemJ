# -*- encoding: utf-8 -*-


import yaml


class YAMLReader(object):

    def __init__(self, filepath=None):
        self.filepath = filepath

    def read(self, filepath=None):
        if filepath is None and self.filepath is None:
            raise Exception('Input please filepath')
        elif filepath is None:
            filepath = self.filepath

        return yaml.load(open(filepath, 'r'))

a = YAMLReader()
print a.read('/home/warmonger/Dropbox/Study/Diploma/Diploma/resources/channels/input_file.yaml')
