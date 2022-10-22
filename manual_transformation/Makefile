all: runtime

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;

condor:
	make -C condor submit;

clean:
	make -C tests clean ;
	make -C condor clean ;

uninstall:
	rm -rf runtime ;

.PHONY: condor
