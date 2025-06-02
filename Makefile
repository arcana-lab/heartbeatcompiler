all: compiler

noelle:
	git clone https://github.com/arcana-lab/noelle.git noelle ;
	cd noelle ; git checkout origin/v14 -b v14 ; git checkout 3f421a706f8fe2d6db61ac8027d5bec20efee857 ;

taskparts:
	git clone https://github.com/mikerainey/taskparts.git taskparts ;
	cd taskparts ; git checkout origin/successor -b successor ; git checkout f83c4880520b2406fa8bf7b8d60402a8e02e8be6 ;

rollforward:
	git clone https://github.com/mikerainey/rollforward.git rollforward ;
	cd rollforward ; git checkout 217d186a47388a84f96fdc2060f91e68c5fab402 ;

heartbeat-linux:
	git clone https://github.com/PrescienceLab/hbc-kernel-module.git heartbeat-linux ;
	cd heartbeat-linux ; git checkout cdae8e13019fd0382f14f3b94cc3fe6d8bcc075d ;

patches: noelle taskparts rollforward
	cp -r patches/* . ;

compiler: noelle taskparts rollforward heartbeat-linux patches
	./build.sh ;

clean:

uninstall:
	rm -rf build ;
	rm -rf *.json ;
	rm -rf noelle taskparts rollforward heartbeat-linux ;

.PHONY: compiler clean
