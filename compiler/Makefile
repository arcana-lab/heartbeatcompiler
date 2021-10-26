all:
	cd src ; ./run_me.sh ;

runtime:
	git clone git@github.com:mikerainey/taskparts.git  runtime ;

clean:
	cd src ; rm -rf build .clangd ; rm -f *.json ;
	cd example ; make clean ;

uninstall:
	rm -rf runtime ;

.PHONY: clean uninstall
