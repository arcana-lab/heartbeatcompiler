all: runtime

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	cd runtime ; git checkout 6ba9dc6de22b68089871c639415a5453367c1d98 ;
	cp -r patches/* . ;

condor:
	make -C condor submit;

clean:
	make -C tests clean ;
	make -C condor clean ;

uninstall:
	rm -rf runtime heartbeat-linux ;

.PHONY: condor
