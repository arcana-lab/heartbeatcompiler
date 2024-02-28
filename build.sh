#!/bin/bash

export CC=clang
export CXX=clang++

root_dir=`git rev-parse --show-toplevel`

# check the cmake binary
command -v cmake3
if test $? -eq 1 ; then
  CMAKE="cmake" ;
else
  CMAKE="cmake3" ;
fi

# build kernel module runtime if it hasn't been build
if ! test -e heartbeat-linux/libhb.so ; then
	cd heartbeat-linux ;
  make libhb ;
  ln -s build/libhb.so . ;
  cd ../ ;
fi

# build noelle if it hasn't been build
if ! test -d noelle/install ; then
  cd noelle ;
  make clean ;
  make src ;
  cd ../ ;
fi

# uninstall
rm -rf build/ ;

# build
mkdir build ;
cd build ;
${CMAKE} -DCMAKE_INSTALL_PREFIX="./" -DCMAKE_BUILD_TYPE=Debug ../ ;
make -j ;
make install ;
cd ../ ;

if ! test -e compile_commands.json ; then
  ln -s build/compile_commands.json ./ ;
fi

# generate enable file
installDir="`realpath .`"
enableFile="enable"

echo "#!/bin/bash" > ${enableFile} ;
echo "" >> ${enableFile} ;
echo "source `realpath .`/noelle/enable ;" >> ${enableFile} ;
