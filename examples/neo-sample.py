# Achilleas Koutsou <achilleas.k@gmail.com>

import neo
from neo import NixIO


blk = neo.Block()
seg = neo.Segment()

st = neo.SpikeTrain([1, 2, 3, 21, 90], units="ms", t_stop=100)

blk.segments.append(seg)
seg.spiketrains.append(st)

with NixIO("spiketrain.nix", "ow") as nixfile:
    nixfile.write_block(blk)
