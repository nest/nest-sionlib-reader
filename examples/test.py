import nest

nest.SetKernelStatus({"local_num_threads": 2, "resolution": .5})

nrns = nest.Create('iaf_cond_alpha', 2)

nest.SetStatus(nrns, 'I_e', 1000.)

meter = nest.Create("multimeter")
nest.SetStatus(meter, {'record_to': ['file'], 'record_from': ['V_m', 'g_ex'], 'interval': .5})
detector = nest.Create("spike_detector")
nest.SetStatus(detector, {'record_to': ['file']})

nest.Connect(meter, nrns)
nest.Connect(nrns, detector)

nest.Simulate(10.)
