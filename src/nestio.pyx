from libcpp.vector cimport vector
from libcpp.string cimport string
from libc.stdint cimport uint64_t, uint32_t, int64_t

from cpython cimport Py_buffer

cdef extern from "nest_reader.h":
    cdef cppclass CDeviceData "DeviceData":
        char* get_raw() except +

        uint64_t gid;
        uint32_t type;
        string name;
        string label;
        vector[string] observables;

        size_t rows;
        size_t values;

    cdef cppclass CNestReader "NestReader":
        CNestReader(string) except +

        CDeviceData* get_device_data_ptr(uint64_t) except +
        vector[uint64_t] list_devices()

        double get_start() except +
        double get_end() except +
        double get_resolution() except +

cdef class DeviceData:
    cdef CDeviceData* entry
    cdef NestReader nest_reader # keep a reference for the parent

    cdef bytes format
    cdef Py_ssize_t shape[1]
    cdef Py_ssize_t strides[1]

    def __cinit__(self, uint64_t gid, NestReader nest_reader):
        self.entry = nest_reader.reader.get_device_data_ptr(gid)
        self.nest_reader = nest_reader

    def __dealloc__(self):
        self.nest_reader = None

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
            return self.entry.label

    property observables:
        def __get__(self):
            return <list> self.entry.observables

    property rows:
        def __get__(self):
            return self.entry.rows

    property values: #TODO
        def __get__(self):
            return self.entry.values

    def __getbuffer__(self, Py_buffer* buffer, int flags):
        try: self.getbuffer_(buffer, flags)
        except Exception as e:
            print(e)
            raise

    cdef getbuffer_(self, Py_buffer* buffer, int flags):
        # underlying object: [gid=uint64, step=int64, offset=double, values....][shape[0]]
        # Length of row: shape[1]
        #TODO: Change buffer meta information
        cdef:
            size_t rows   = self.entry.rows
            size_t values = self.entry.values #TODO: For long and double
            Py_ssize_t itemsize = sizeof(uint64_t) + sizeof(int64_t) + sizeof(double) + values*sizeof(double)
        #TODO: Change computation of itemsize

        self.strides[0] = itemsize
        self.shape[0] = rows
        self.format = "=Q=q=d".encode()
        if values > 0: self.format += "={}d".format(values).encode()
        #TODO: self.format += "={}q".format(num_long_values).encode()

        buffer.buf = self.entry.get_raw()
        buffer.format = <char*> self.format
        buffer.internal = NULL
        buffer.itemsize = itemsize
        buffer.len = itemsize * rows
        buffer.ndim = 1
        buffer.obj = self
        buffer.readonly = 0
        buffer.shape = self.shape
        buffer.strides = self.strides
        buffer.suboffsets = NULL

    def __releasebuffer__(self, Py_buffer* buffer):
        self.format = None

cdef class NestReader:
    cdef CNestReader* reader
    def __cinit__(self, str filename):
        self.reader = new CNestReader(filename.encode())

    def __dealloc__(self):
        del self.reader
        
    def __iter__(self):
        cdef:
            vector[uint64_t] gids = self.reader.list_devices()
            CDeviceData* device_data_ptr;
            uint64_t g

        for g in gids:
            yield DeviceData(g, self)

    def __getitem__(self, uint64_t gid):
        return DeviceData(gid, self)

    property t_start:
        def __get__(self):
            return self.reader.get_start()

    property t_end:
        def __get__(self):
            return self.reader.get_end()

    property resolution:
        def __get__(self):
            return self.reader.get_resolution()
