all: compiler

compiler: runtime
	cd src ; ./run_me.sh ;

runtime:
	git clone git@github.com:mikerainey/taskparts.git runtime ;
	cd runtime ; git checkout 4b21176a6f0707fb43b810fed3b8ff0e7796531c ;

clean:
	cd src ; rm -rf build ; rm -f *.json ;
	cd example ; make clean ;
	find ./ -name .cache -exec rm -rv {} +
	find ./ -name .clangd -exec rm -rv {} +

uninstall: clean
	rm -rf runtime ;

.PHONY: clean uninstall compiler
