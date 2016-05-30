#!/usr/bin/bash
set -e
set -x

export DEST_OS=win32
export CC=gcc
export CXX=g++
export PKG_CONFIG=/usr/bin/pkg-config
export TOPDIR=$(pwd)

env
which gcc
which g++
g++ --version

mkdir ${TOPDIR}/build
mkdir ${TOPDIR}/install

export PATH=${PATH}:${TOPDIR}/install/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TOPDIR}/install/lib

cd ${TOPDIR}/build
cmake -G "MSYS Makefiles" -DCMAKE_INSTALL_PREFIX:PATH=${TOPDIR}/install -DOptionParser_INSTALL_EXAMPLES=ON ${TOPDIR}/
make
make install
cd ${TOPDIR}/install
optionExample --help
${TOPDIR}/src/shellScriptExample --help
