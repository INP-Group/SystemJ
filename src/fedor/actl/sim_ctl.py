#!/usr/bin/env python
# -*- coding: utf-8 -*-

from cdr_wrapper import *

# class for simultaneous control if identical channels
#

class sim_ctl:
    def __init__(self, cnames, valChangeCB=None):
        self.cdr = cdr()
        self.cnames = cnames
        self.valChangeCB = valChangeCB

        self.callback = lambda h, v, p: self.cb(h, v)

        self.cx_chans = []
        for x in self.cnames:
            self.cx_chans.append(self.cdr.RegisterSimpleChan(x, self.callback))

        self.prev_vals = [None for x in self.cnames]
        self.vals = [None for x in self.cnames]

        self.val = 0

        self.is_switching = False
        self.switching_to = 0


    def cb(self, handle, val):

        ind = self.cx_chans.index(handle)
        self.prev_vals[ind] = self.vals[ind]
        self.vals[ind] = val

        if val != self.prev_vals[ind] and not self.is_switching:
            self.is_switching = True
            self.switching_to = val
            self.cset(val)

        if self.is_switching:
            if self.check_state(self.switching_to):
                self.is_switching = False
                if callable(self.valChangeCB):
                    self.valChangeCB()

        return (0)


    def check_state(self, val):
        for x in self.vals:
            if x != val:
                return (False)
        return (True)


    def cset(self, val):
        for x in self.cx_chans:
            self.cdr.SetSimpleChanVal(x, val)
        self.val = val

