from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

# required to get numpy includes
import numpy as np

# set correct SIONlib include and link paths
from subprocess import Popen, PIPE

proc = Popen('sionconfig --cflags --gcc --ser'.split(), stdout=PIPE)
proc.wait()
CFLAGS = proc.communicate()[0].strip().split()

proc = Popen('sionconfig --libs --gcc --ser'.split(), stdout=PIPE)
proc.wait()
LDFLAGS = proc.communicate()[0].strip().split()

setup(
        cmdclass = {'build_ext': build_ext},
        ext_modules = [
            Extension("nestio",
                ["wrapper.pyx", "sion_reader.cpp", "raw_memory.cpp", "nestio.cpp"],
                language='c++',
                extra_compile_args=CFLAGS + ["-std=c++11"],
                extra_link_args=LDFLAGS + ["-std=c++11"]
                )
            ],
        include_dirs = [np.get_include()],
        )
