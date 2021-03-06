#!/usr/bin/python3

import nest

nest.SetKernelStatus({"local_num_threads": 4, "resolution": .5, 'recording': {'recording_backend': 'RecordingBackendSIONlib'}})

nrns = nest.Create('iaf_psc_alpha', 16)

nest.SetStatus(nrns, 'I_e', 1000.)

meter = nest.Create("multimeter", params={'stop': 3.})
nest.SetStatus(meter, {'record_from': ['V_m'], 'interval': .5, 'stop': 1000.})
detector = nest.Create("spike_detector")
nest.SetStatus(detector, {'start': 0., 'stop': 1200.})

nest.Connect(meter, nrns)
nest.Connect(nrns, detector)

nest.Simulate(1500.)
