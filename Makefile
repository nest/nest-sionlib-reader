SIONCONFIG = sionconfig
SION_SER_LIBS =  `$(SIONCONFIG) --libs --ser --be --64`
SION_SER_CFLAGS = `$(SIONCONFIG) --cflags --ser --be --64`

read-sion-file:
	g++ -o read-sion-file read-sion-file.cpp $(SION_SER_CFLAGS) $(SION_SER_LIBS)

clean:
	rm read-sion-file
