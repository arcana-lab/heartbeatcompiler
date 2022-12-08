all: runtime

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	cd runtime ; git checkout 43c16f6430ebe32cb3d31bb86aa2038e757375eb ;
	git clone https://github.com/nickwanninger/heartbeat-linux.git heartbeat-linux ;
	cd heartbeat-linux ; make ; make kmod ;

condor:
	make -C condor submit;

clean:
	make -C tests clean ;
	make -C condor clean ;

uninstall:
	rm -rf runtime heartbeat-linux ;

.PHONY: condor
