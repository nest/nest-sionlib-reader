SIONCONFIG = sionconfig
SION_SER_LIBS =  `$(SIONCONFIG) --libs --ser --be --64 --gcc`
SION_SER_CFLAGS = `$(SIONCONFIG) --cflags --ser --be --64 --gcc`

read-sion-file:
	g++ -o read-sion-file read-sion-file.cpp dependencies/argparse/OptionParser.cpp $(SION_SER_CFLAGS) $(SION_SER_LIBS)

clean:
	rm read-sion-file
