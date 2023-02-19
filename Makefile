all: runtime link

runtime:
	git clone https://github.com/mikerainey/taskparts.git runtime ;
	git clone https://github.com/mikerainey/rollforward.git rollforward ;
	cp -r patches/* . ;

link:
	make -C benchmarks link ;
	make -C manual_transformation link ;
	make -C compiler link ;

clean:
	make -C benchmarks clean ;
	make -C manual_transformation clean ;
	make -C compiler clean ;

uninstall:
	rm -rf runtime rollforward ;

.PHONY: clean 
