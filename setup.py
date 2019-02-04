from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

# required to get numpy includes
import numpy as np

# set correct SIONlib include and link paths
from subprocess import Popen, PIPE

proc = Popen('sionconfig --cflags --gcc --ser'.split(), stdout=PIPE)
proc.wait()
CFLAGS = list(x.decode() for x in proc.communicate()[0].strip().split())

proc = Popen('sionconfig --libs --gcc --ser'.split(), stdout=PIPE)
proc.wait()
LDFLAGS = list(x.decode() for x in proc.communicate()[0].strip().split())

setup(
    name="nestio",
    version="0.1.0",
    cmdclass = {'build_ext': build_ext},
    ext_modules = [
        Extension("nestio",
            ["src/nestio.pyx", "src/sion_reader.cpp", "src/raw_memory.cpp", "src/nest_reader.cpp"],
            language='c++',
            extra_compile_args=CFLAGS + ["-std=c++11"],
            extra_link_args=LDFLAGS + ["-std=c++11"]
            )
        ],
    include_dirs = [np.get_include()],
    )
