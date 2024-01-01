all: patches build-noelle build-compiler link

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	cd runtime ; git checkout 3188988c56ff2764809c4b7676632c4eea175d5e ;
	git clone https://github.com/mikerainey/rollforward.git rollforward ;
	cd rollforward ; git checkout 217d186a47388a84f96fdc2060f91e68c5fab402 ;

noelle:
	git clone https://github.com/arcana-lab/noelle.git noelle ;
	cd noelle ; git checkout origin/v14 -b v14 ; git checkout 6e93d0da2c3dc2da00f65ebe43f4ebb707ed75e8 ;

matrix-market:
	git clone https://github.com/cwpearson/matrix-market.git matrix-market ;

patches: runtime noelle matrix-market
	# cp -r patches/* . ;

build-noelle: noelle
	cd noelle ; make clean ; make uninstall ; make src ;

build-compiler: noelle
	cd compiler ; make clean ; make compiler ;

link:
	make -C benchmarks link ;

clean:
	make -C benchmarks clean ;
	make -C compiler clean ;

uninstall:
	rm -rf runtime rollforward matrix-market noelle ;

.PHONY: clean
