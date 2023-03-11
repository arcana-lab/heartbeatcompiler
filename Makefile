all: runtime link

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	git clone https://github.com/mikerainey/rollforward.git rollforward ;
	cp -r patches/* . ;

link:
	make -C benchmarks link ;

clean:
	make -C benchmarks clean ;

uninstall:
	rm -rf runtime rollforward ;

.PHONY: clean 
