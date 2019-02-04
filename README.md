# nestio-tools

## Installation

First install SIONlib; for instructions, see <http://www.fz-juelich.de/jsc/sionlib>.

To compile and build nestio-tools, just enter

```bash
$ python3 setup.py install
```

## Usage

The build will generate a dynamic library (.so). Set your PYTHONPATH and LD_LIBRARY_PATH to the directory that contains the generated .so file. Then, launch Python and simply

```python
import nestio
```

You can process files created by NEST's `SIONLogger` as follows:

```python
nestio.Reader("/home/jochen/data.sion")
```
