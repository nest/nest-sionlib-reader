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
sionlib_rec_backend_version={}
nest_version={}
""", f.t_start, f.t_end, f.resolution, f.sionlib_rec_backend_version, f.nest_version
)
    
for i in f:
    write(
"""
> gid={}
> type={}
> name={}
> label={}
> origin={}
> t_start={}
> t_stop={}
> rows={}
> double_n_val={}
> long_n_val={}
""", i.gid, i.dtype, i.name, i.label, i.origin, i.t_start, i.t_stop, i.rows, i.double_n_val, i.long_n_val
    )

    for j in i.double_observables:
        write(">> double_observable={}\n", j)
    for j in i.long_observables:
        write(">> long_observable={}\n", j)

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
        double_v = j[3] if i.double_n_val else None
        long_v = j[4] if i.long_n_val else None
        write("""
>> gid={}
>> step={}
>> offset={}
>> double_values={}
>> long_values={}
        """, gid, step, offset, double_v, long_v
        )
