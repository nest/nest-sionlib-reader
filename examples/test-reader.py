import nestio

f = nestio.Reader("examples/output.sion")
for i in f:
    print i.label
    
