all: compiler

compiler:
	cd src ; ./run_me.sh ;

link:
	make -C benchmarks link ;

clean:
	cd src ; rm -rf build ; rm -f *.json ;
	cd example ; make clean ;
	find ./ -name .cache -exec rm -rv {} +
	find ./ -name .clangd -exec rm -rv {} +
	make -C benchmarks clean ;

.PHONY: compiler clean
