all: patches build-noelle build-compiler

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	git clone https://github.com/mikerainey/rollforward.git rollforward ;

noelle:
	git clone https://github.com/yiansu/noelle.git noelle ;
	cd noelle ; git checkout origin/llvm15 -b llvm15 ;

matrix-matrix:
	git clone https://github.com/cwpearson/matrix-market.git matrix-market ;

patches: runtime noelle matrix-market
	cp -r patches/* . ;

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
	rm -rf runtime rollforward noelle ;

.PHONY: clean 
