from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.pair cimport pair
import numpy as np
cimport numpy as np
from cython.view cimport array as cvarray


cdef extern from "nestio.h":
	cdef cppclass CDeviceData "DeviceData":
		CDeviceData(pair[unsigned long, unsigned long]) except +

		double* get_data_ptr();

		pair[unsigned long, unsigned long] shape;
		string name;
		string label;
		int gid;

	cdef cppclass CSIONFile "SIONFile":
		CSIONFile(string) except +
		CDeviceData* get_device_data(int) except +
		vector[int] list_devices()

cdef class DeviceData:
	cdef CDeviceData* entry

	def __cinit__(self, long entry_ptr):
		self.entry = <CDeviceData*> entry_ptr;

	property data:
		def __get__(self):
			cdef pair[unsigned long, unsigned long] shape = self.entry.shape
			cdef double* data_ptr = self.entry.get_data_ptr()
			numpy_array = np.asarray(<np.float64_t[:shape.first,:shape.second]> data_ptr)
			return numpy_array
	
	property name:
		def __get__(self):
			return self.entry.name
	
	property label:
		def __get__(self):
			return self.entry.name
	
	property gid:
		def __get__(self):
			return self.entry.gid

cdef class NESTFile:
	cdef CSIONFile* sion_file
	def __init__(self, filename):
		self.sion_file = new CSIONFile(filename)
	
	def __iter__(self):
		cdef vector[int] gids = self.sion_file.list_devices()
		cdef CDeviceData* device_data_ptr;
		for g in gids:
			device_data_ptr = self.sion_file.get_device_data(g)
			yield DeviceData(<long> device_data_ptr)

	def __getitem__(self, device_gid):
		cdef CDeviceData* device_data_ptr = self.sion_file.get_device_data(device_gid)
		dev_data = DeviceData(<long> device_data_ptr)
		return dev_data
