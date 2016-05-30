#!/usr/bin/bash
#set -e
set -x

export DEST_OS=win32
export CC=C:/MinGW/bin/gcc
export CXX=C:/MinGW/bin/g++
export PKG_CONFIG=/usr/bin/pkg-config
export TOPDIR=$(pwd)

which gcc
which g++
ls C:
ls /usr/bin
env
ls C:\MinGW
ls C:/MinGW

mkdir ${TOPDIR}/build
mkdir ${TOPDIR}/install

export PATH=${PATH}:${TOPDIR}/install/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TOPDIR}/install/lib

cd ${TOPDIR}/build
cmake -DCMAKE_INSTALL_PREFIX:PATH=${TOPDIR}/install -DOptionParser_INSTALL_EXAMPLES=ON ${TOPDIR}/
make
make install
cd ${TOPDIR}/install
optionExample --help
${TOPDIR}/src/shellScriptExample --help
