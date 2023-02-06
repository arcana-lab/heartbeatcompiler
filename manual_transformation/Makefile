all: runtime

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	cd runtime ;
	git clone https://github.com/mikerainey/rollforward.git rollforward ;
	cp -r patches/* . ;

condor:
	make -C condor submit;

clean:
	make -C benchmarks clean ;
	make -C tests clean ;
	make -C condor clean ;

uninstall:
	rm -rf runtime rollforward ;

.PHONY: condor
