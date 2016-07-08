#!/usr/bin/python3

from sys import stdout
import nestio
import numpy as np

def write(fstr, *vals):
    stdout.write(fstr.format(*vals))

f = nestio.NestReader("output.sion")

write(
"""\
start={}
end={}
resolution={}
""", f.t_start, f.t_end, f.resolution
)
    
for i in f:
    write(
"""
> gid={}
> type={}
> name={}
> label={}
> rows={}
> values={}
""", i.gid, i.dtype, i.name, i.label, i.rows, i.values
    )

    for j in i.observables:
        write(">> observable={}\n", j)

    A = np.asarray(i)
    write("""
>> numpy={}
""", np.array_repr(A)
    )

    for j in A:
#        write("""
#>> row={}
#""", np.array_repr(j))
        gid, step, offset = j[0], j[1], j[2]
        v = j[3] if i.values else None
        write("""
>> gid={}
>> step={}
>> offset={}
>> values={}
""", gid, step, offset, v
        )
