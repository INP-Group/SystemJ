

import ctypes

class ByteSegmentsArray:
    """
    Array of int items with predefined length.
    Could be passed to any C function as void* (use AsCVoidPointer method)
    Will be deleted by python automatically

    EXAMPLE:
    buf = ByteSegmentsArray(100, ByteSegmentsArray.INT16)
    testlib.fill_buf.argtypes = [ctypes.c_int, int_p]
    testlib.fill_buf(50, buf.AsCVoidPointer())
    print buf.AsPythonList()

    use example
    #buf = ByteSegmentsArray(10, ByteSegmentsArray.INT8)
    #cdr.CdrGetSimpleBigcData(vdc200_1.handle, 0, 10, buf.AsCVoidPointer())
    #print buf.AsPythonList()

    """
    # This types do not cover our needs!!! no uints
    INT8 = 1
    INT16 = 2
    INT32 = 4
    INT64 = 8
    def __init__(self, length, size=INT8):
        self.data_type = None
        if size==self.INT8: self.data_type = ctypes.c_int8
        if size==self.INT16: self.data_type = ctypes.c_int16
        if size==self.INT32: self.data_type = ctypes.c_int32
        if size==self.INT64: self.data_type = ctypes.c_int64
        self.c_data = (self.data_type * length)()
        self.length = length

    def AsCVoidPointer(self):
        """
        Get pointer to C data, for passing as argument by pointer to cytpes functions
        """
        return ctypes.cast(self.c_data, ctypes.c_void_p)

    def AsPythonList(self, length=None, offset=0):
        """
        Convert C data to python list, could use length and offset parameters.
        """
        ### Performance: 2.5M items array converts during 1 second
        ret = []
        if length is None: length = self.length
        for i in range(0, length):
            val = int(self.c_data[i + offset])
            if val is not None: ret+=[val]
        return ret

    def __getitem__(self, item):
        """
        Just to address i-th item of the obj

        EXAMPLE:
        print buf[i]
        """
        if type(item) != int or item >= self.length:
            return None
        return int(self.c_data[item])
