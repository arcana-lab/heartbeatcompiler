all: build link

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	git clone https://github.com/mikerainey/rollforward.git rollforward ;

noelle:
	git clone https://github.com/arcana-lab/noelle.git noelle ;
	cd noelle ; git checkout e0d62d4ab4704ae16b7afdcad285c638ff143621 ;

patches: runtime noelle
	cp -r patches/* . ;

build: patches
	cd noelle ; make clean ; make uninstall ; make src ;
	cd compiler ; make clean ; make compiler ;

link:
	make -C benchmarks link ;

clean:
	make -C benchmarks clean ;

uninstall:
	rm -rf runtime rollforward noelle ;

.PHONY: clean 
