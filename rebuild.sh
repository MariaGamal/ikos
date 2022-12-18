BASEDIR=$(dirname "$0")

# make sure the script is run from the right directory
cd $BASEDIR

# clean up
rm -rf build

# build ikos
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=. ..
make -j7
make install
