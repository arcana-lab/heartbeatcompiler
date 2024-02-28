all: compiler

noelle:
	git clone https://github.com/arcana-lab/noelle.git noelle ;
	cd noelle ; git checkout origin/v14 -b v14 ; git checkout 3f421a706f8fe2d6db61ac8027d5bec20efee857 ;

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	cd runtime ; git checkout 3188988c56ff2764809c4b7676632c4eea175d5e ;

rollforward:
	git clone https://github.com/mikerainey/rollforward.git rollforward ;
	cd rollforward ; git checkout 217d186a47388a84f96fdc2060f91e68c5fab402 ;

heartbeat-linux:
	git clone https://github.com/PrescienceLab/hbc-kernel-module.git heartbeat-linux ;
	cd heartbeat-linux ; git checkout cdae8e13019fd0382f14f3b94cc3fe6d8bcc075d ;

patches: noelle runtime rollforward
	cp -r patches/* . ;

compiler: noelle runtime rollforward heartbeat-linux patches
	./build.sh ;

clean:

uninstall:
	rm -rf build ;
	rm -rf *.json ;
	rm -rf noelle runtime rollforward heartbeat-linux ;

.PHONY: compiler clean
