#!/bin/bash -x

mkdir /opt/OptionParser

mkdir build
pushd build

cmake -DCMAKE_INSTALL_PREFIX=/opt/OptionParser -DINSTALL_DOCUMENTATION=on ..
make
make doc
make install

popd

cp -rv /opt/OptionParser/share/doc/OptionParser/html .
chmod -R a+rwX ./html
