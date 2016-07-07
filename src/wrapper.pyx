from libcpp.vector cimport vector
from libcpp.string cimport string
from libc.stdint cimport uint64_t, uint32_t

import numpy as np
cimport numpy as np
from cython.view cimport array as cvarray


cdef extern from "nestio.h":
    cdef cppclass CDeviceData "DeviceData":
        CDeviceData(size_t, size_t) except +

        char* get_data() except +

        uint64_t gid;
        uint32_t type;
        string name;
        string label;
        vector[string] observables;

        size_t rows;
        size_t values;

    cdef cppclass CNestReader "NestReader":
        CNestReader(string) except +

        CDeviceData* get_device_data(int) except +
        vector[int] list_devices()

        double get_start() except +
        double get_end() except +
        double get_duration() except +

class DevArray(np.ndarray):
    # need to keep a reference to the DeviceData object
    def __new__(cls, view, device, dtype):
        obj = np.asarray(view, dtype=dtype).view(cls)
        obj.device = device
        return obj

    def __array_finalize__(self, obj):
        if obj is None: return
        self.device = getattr(obj, 'device', None)

cdef class DeviceData:
    cdef CDeviceData* entry
    cdef object nestReader # keep a reference for the parent

    def __cinit__(self, long entry_ptr, NestReader nestReader):
        self.entry = <CDeviceData*> entry_ptr
        self.nestReader = nestReader

    def __dealloc__(self):
        self.nestReader = None

    property data:
        def __get__(self):
            # underlying object: [gid=uint64, int64=step, offset=double, values....][shape[0]]
            # Length of row: shape[1]
            cdef size_t rows = self.entry.rows
            cdef size_t values = self.entry.values
            cdef char* data = self.entry.get_data()
            cdef str dtype = "=Q=q=d={}d".format(values)
            cdef tuple dim = (rows,)
            cdef cvarray buf = cvarray(data, shape=dim, dtype=dtype)

            return DevArray(buf, self, dtype)

    property gid:
        def __get__(self):
            return self.entry.gid

    property dtype:
        def __get__(self):
            return self.entry.type

    property name:
        def __get__(self):
            return self.entry.name

    property label:
        def __get__(self):
            return self.entry.name

    property observables:
        def __get__(self):
            return <list> self.entry.observables

cdef class NestReader:
    cdef CNestReader* reader
    def __init__(self, filename):
        self.reader = new CNestReader(filename)

    def __dealloc__(self):
        del self.reader
        
    def __iter__(self):
        cdef vector[int] gids = self.reader.list_devices()
        cdef CDeviceData* device_data_ptr;
        for g in gids:
            device_data_ptr = self.reader.get_device_data(g)
            yield DeviceData(<long> device_data_ptr)

    def __getitem__(self, device_gid):
        cdef CDeviceData* device_data_ptr = self.reader.get_device_data(device_gid)
        dev_data = DeviceData(<long> device_data_ptr)
        return dev_data

    property t_start:
        def __get__(self):
            return self.reader.get_start()

    property t_end:
        def __get__(self):
            return self.reader.get_end()

    property duration:
        def __get__(self):
            return self.reader.get_duration()
