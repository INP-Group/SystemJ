# -*- encoding: utf-8 -*-


import ConfigParser

class ConfigChannel(object):

    def __init__(self):
        self.config = ConfigParser.ConfigParser()

    def getConfigSection(self, section):
        dict1 = {}
        options = self.config.options(section)
        for option in options:
            try:
                dict1[option] = self.config.get(section, option)
                if dict1[option] == -1:
                    print ("skip: %s" % option)
            except:
                print("exception on %s!" % option)
                dict1[option] = None
        return dict1
