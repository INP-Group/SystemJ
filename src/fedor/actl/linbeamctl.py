#!/usr/bin/env python
# -*- coding: utf-8 -*-

# this class on/off linac beam with cav_h corrector
# if requested beam off:
#
#

import math

#this class works with CDRWrapper
from cdr_wrapper import *


#names of corrector to close beam
beam_ctl_set = "linmagx.magcor.cav_h_set"
beam_ctl_meas = "linmagx.magcor.cav_h_mes"

#current values to determine situalions (mA)
beam_off_val = -4600
beam_on_threshold = 3000
beam_off_threshold = -3000
#admissible error levels
err_set = 5
err_meas = 50
# cx cycles to wait for channel value change
beam_ctl_timeout = 10


class linbeamctl:
    def __init__(self, wrapper, beam_on_cb=None, beam_off_cb=None, lost_control_cb=None):
        #init for work with CX 
        self.wrapper = wrapper
        self.callback = wrapper.MakeCdrCallback(self.newdata_cb)
        self.set_chan = wrapper.CdrRegisterSimpleChan(beam_ctl_set, None, None)
        self.meas_chan = wrapper.CdrRegisterSimpleChan(beam_ctl_meas, self.callback, None)

        # init for variables. None = unknown state
        self.oper_corr_val = None  # save here operator value
        self.beam_status = -1  # -1 = unknown, 0 = off , 1 = on
        self.requested_status = -1  # -1 = no requests, 0/1 - requested beam status
        self.set_val = 0  # set value
        self.meas_val = 0  # measured value
        self.switch_time = 0  # cx cycles counter to check timeout

        # user function
        self.beam_on_cb = beam_on_cb
        self.beam_off_cb = beam_off_cb
        self.lost_control_cb = lost_control_cb
        self.reason = ""


    def newdata_cb(self, handle, val, params):
        self.set_val = self.wrapper.CdrGetSimpleChanVal(self.set_chan)
        self.meas_val = self.wrapper.CdrGetSimpleChanVal(self.meas_chan)

        #initialize beam status
        if self.requested_status == -1:
            if self.oper_corr_val == None:
                self.oper_corr_val = math.fabs(self.set_val)
            if self.meas_val > beam_on_threshold and self.beam_status != 1:
                self.beam_status = 1
                self.reason = 'unknown'
                self.beam_on_cb()
                return (0)

            if self.meas_val < beam_off_threshold and self.beam_status != 0:
                self.beam_status = 0
                self.reason = 'unknown'
                self.beam_off_cb()
                return (0)

        #processing beam status change 
        if self.requested_status == 0:
            if math.fabs(self.set_val - beam_off_val) < err_set or \
                            math.fabs(self.meas_val - beam_off_val) < err_meas:
                self.switch_time += 1
            else:
                self.requested_status = -1  # request done
                self.beam_status = 0  # beam now off
                self.reason = 'request done'
                self.beam_off_cb()
                return (0)

        if self.requested_status == 1:
            if math.fabs(self.set_val - self.oper_corr_val) < err_set or \
                            math.fabs(self.meas_val - self.oper_corr_val) < err_meas:
                self.switch_time += 1
            else:
                self.requested_status = -1  # request done
                self.beam_status = 1  # beam now on
                self.reason = "request done"
                self.beam_on_cb()
                return (0)

        if self.switch_time == beam_ctl_timeout:
            self.beam_status = -1
            self.requested_status = -1
            self.reason = 'timeout'
            self.lost_control_cb()
        return (0)


    def beam_off(self):
        self.requested_status = 0  #request beam off
        if self.beam_status == 0:
            self.requested_status = -1
            self.reason = 'beam already off'
            self.beam_off_cb()
            return
        self.set_val = self.wrapper.CdrGetSimpleChanVal(self.set_chan)
        self.oper_corr_val = self.set_val
        self.wrapper.CdrSetSimpleChanVal(self.set_chan, beam_off_val)
        self.switch_time = 0


    def beam_on(self):
        self.requested_status = 1  #request beam off
        if self.beam_status == 1:
            self.requested_status = -1
            self.reason = 'beam already on'
            self.beam_on_cb()
            return
        self.wrapper.CdrSetSimpleChanVal(self.set_chan, self.oper_corr_val)
        self.switch_time = 0











     