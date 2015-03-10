# -*- encoding: utf-8 -*-

import ctypes
import sys


class CdrWrapper(object):
    """
    CDR Wrapper class ver0.7, see use example below
    """
    # storage for regular channels
    callbacks = {}  # users callbacks (key=chan, value = [cb1, ....])
    registered_chans = {}  #
    c_callbacks = {}  # storage for self-generated C-style callbacks

    # storage for big channels
    bigc_callbacks = {}  # users callbacks (key=chan, value = [cb1, ....])
    registered_bigcs = {}  #
    bigc_c_callbacks = {}  # storage for self-generated C-style callbacks


    def __init__(self, absolute_lib_path='libCdr4PyQt.so', opt_argv0=None):
        """
        Loads library
        absolute_lib_path - path to CDR library .so or .dll file
        opt_argv0 - optionally argv0 param, defaultly gets sys.arv[0]
        """

        if opt_argv0 is None:
            self.opt_argv0 = sys.argv[0]

        self.library = ctypes.CDLL(absolute_lib_path)
        self.argv0 = opt_argv0

    def MakeChanCallback(self, python_callable):
        """
        Returns cdr callback from python callable function.
        python_callable - function that takes handle(integer), value(double), private_params(object) and returns error code (integer, TODO: check "0 if OK")

        EXAMPLE:
            def test_cb_py(handle, val, params):
                print "Python Callback:", handle, val, params
                return 0

        INNER: wraps python function by ctypes descriptor for c++
        """
        CB_FUNC = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.c_void_p)
        ret = CB_FUNC(python_callable)
        return ret 

    def MakeChanCallbackIfNeeded(self, callback):
        ret = callback
        fpy = lambda x, y, z: 0
        if type(callback) == type(fpy):
            ret = self.MakeChanCallback(callback)
        return ret

    def CreateChanCallback(self, name):
        def cb(h, v, p):
            if self.callbacks.has_key(name):
                for x in self.callbacks[name]:
                    x(h, v, p)
            return 0
        return cb

    def RegisterSimpleChan(self, name, callback, private_params=None):
        """
        Registers cdr callback by specified name event(???), with optional private params

        2DO: depricated dict.has_key() used python 2.6.6 do not support .in()
        """
        if self.registered_chans.has_key(name):
            chan = self.registered_chans[name]
            if callable(callback):
                self.callbacks[name].append(callback)
            return chan

        cb = self.CreateChanCallback(name)
        c_cb = self.MakeChanCallback(cb)

        self.library.CdrRegisterSimpleChan.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p]
        ret = self.library.CdrRegisterSimpleChan(name, self.argv0, c_cb, private_params)
        if ret < 0:
            raise Exception("Error while Registering Simple Channel Callback, errcode: %s" % ret)

        #We need to keep reference to c_callbacks or Garbage Collector will destroy it.
        self.c_callbacks[name] = c_cb
        if callable(callback):
            self.callbacks[name] = [callback]
        self.registered_chans[name] = ret

        return ret

    def SetSimpleChanVal(self, handle, val):
        """
        Sets Simple Channel Value by handle
        handle - int, id of the channel
        val - double, value to set
        """
        self.library.CdrSetSimpleChanVal.restype = ctypes.c_int
        self.library.CdrSetSimpleChanVal.argtypes = [ctypes.c_int, ctypes.c_double]
        ret = self.library.CdrSetSimpleChanVal(handle, val)  # TODO: check if ret is ctypec.c_int
        if ret != 0:
            raise Exception("Error while Setting Simple Channel Value, errcode: %s" % ret)
        return ret

    def GetSimpleChanVal(self, handle):
        """
        Returns Simple Channel Value, gotten by handle
        handle - int, id of the channel
        """
        self.library.CdrGetSimpleChanVal.restype = ctypes.c_int
        self.library.CdrGetSimpleChanVal.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_double)]
        val = ctypes.c_double(0.0)
        ret = self.library.CdrGetSimpleChanVal(handle, ctypes.byref(val))
        if ret != 0:
            raise Exception("Error while Getting Simple Channel Value, errcode: %s" % ret)
        return val.value


    #############################################
    def MakeBigcCallback(self, python_callable):
        """
        Returns cdr callback from python callable function.
        python_callable - function that takes handle(integer), private_params(object) and returns error code
        (integer, TODO: check "0 if OK")

        EXAMPLE:
            def test_cb_py(handle, params):
                print "Python Callback:", handle, params
                return 0

        INNER: wraps python function by ctypes descriptor for c++
        """
        # TODO: may be better to define callback prototype ones in class but not with callback
        bigCB_FUNC = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_void_p)
        ret = bigCB_FUNC(python_callable)
        return ret 

    def MakeBigcCallbackIfNeeded(self, callback):
        ret = callback
        fpy = lambda x,y: x
        if type(callback) == type(fpy):
             ret = self.MakeBigcCallback(callback)
        return ret

    def CreateBigcCallback(self, name):
        def cb(h, p):
            if self.bigc_callbacks.has_key(name):
                for x in self.bigc_callbacks[name]:
                    x(h, p)
            return 0
        return cb

    def RegisterSimpleBigc(self, name, max_datasize, callback, private_params=None):
        """
        Registers cdr callback by specified name event(???), with optional private params
        """
        if self.registered_bigcs.has_key(name):
            bigc = self.registered_bigcs[name]
            if callable(callback):
                self.bigc_callbacks[name].append(callback)
            return bigc

        cb = self.CreateBigcCallback(name)
        c_cb = self.MakeBigcCallbackIfNeeded(cb)

        self.library.CdrRegisterSimpleBigc.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
        ret = self.library.CdrRegisterSimpleBigc(name, self.argv0, max_datasize, c_cb, private_params)
        if ret < 0:
            raise Exception("Error while Registering Simple BigChan Callback, errcode: %s" % ret)

        #We need to keep reference to c_callbacks or Garbage Collector will destroy it.
        self.bigc_c_callbacks[name] = c_cb
        if callable(callback):
            self.bigc_callbacks[name] = [callback]
        self.registered_bigcs[name] = ret
        return ret

    def GetSimpleBigcData(self, handle, byte_offset, bytes_count, buf_void_p):
        """
        Returns count of bytes read as Simple Bigc Data by handle
        handle - int, id of the bigc
        byte_offset - int, offset of bytes to be read
        bytes_count - int, count of bytes to be read from offset
        buf_void_p - void_p pointer to the buffer
        IMPORTANT: Use this function inside callback, called by QT core by registration of CdrRegisterSimpleBigc, otherwise data could be overwritten
        """
        self.library.CdrGetSimpleBigcData.restype = ctypes.c_int
        self.library.CdrGetSimpleBigcData.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p]
        read_count = self.library.CdrGetSimpleBigcData(handle, byte_offset, bytes_count, buf_void_p)
        if read_count < 0:
            raise Exception("Error while Getting Simple BigChan Data, errcode: %s" % read_count)
        return read_count

    def SetSimpleBigcParam(self, handle, n, val):
        """
        Sets Simple Bigc Param by handle
        handle - int, id of the bigc
        n - int, id of parameter to set
        val - int, param value to set
        """
        self.library.CdrSetSimpleBigcParam.restype = ctypes.c_int
        self.library.CdrSetSimpleBigcParam.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
        ret = self.library.CdrSetSimpleBigcParam(handle, n, val)
        if ret < 0:
            raise Exception("Error while Setting Simple BigChan Param, errcode: %s" % ret)
        return ret

    def GetSimpleBigcParam(self, handle, n):
        """
        Returns Simple Bigc Param, gotten by handle
        handle - int, id of the bigc
        n - int, id of parameter to get
        """
        self.library.CdrGetSimpleBigcParam.restype = ctypes.c_int
        self.library.CdrGetSimpleBigcParam.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
        val = ctypes.c_int(0)
        ret = self.library.CdrGetSimpleBigcParam(handle, n, ctypes.byref(val))
        if ret < 0:
            raise Exception("Error while Getting Simple BigChan Param, errcode: %s" % ret)
        return val.value


class Singleton(type):
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


#this work with python2
class Cdr(CdrWrapper):
    __metaclass__ = Singleton
