all: runtime

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	cd runtime ; git checkout 4b21176a6f0707fb43b810fed3b8ff0e7796531c ;

condor:
	make -C condor submit;

clean:
	make -C tests clean ;
	make -C condor clean ;

uninstall:
	rm -rf runtime ;

.PHONY: condor
