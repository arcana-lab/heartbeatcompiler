all: compiler

compiler:
	cd src ; ./run_me.sh ;

clean:
	cd src ; rm -rf build ; rm -f *.json ;
	cd example ; make clean ;
	find ./ -name .cache -exec rm -rv {} +
	find ./ -name .clangd -exec rm -rv {} +

.PHONY: compiler clean
