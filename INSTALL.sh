#!/bin/bash

# Create Build direction
mkdir build
cd build

# Clone Cypress and build it
git clone https://github.com/hbp-unibi/cypress
cd cypress
mkdir build 
mkdir install
cd build
# Until current branch gets pulled into master
git checkout smaller_changes
cmake .. -DCMAKE_INSTALL_PREFIX:PATH=../install -DSTATIC_LINKING=FALSE
make
make install

# Back to build directory
cd ../../
# Build non-statically with local cypress build
cmake .. -DCypress_DIR=build/cypress/install/share/cmake/Cypress -DCYPRESS_LIBRARY=cypress/install/lib/libcypress.a -DCYPRESS_INCLUDE_DIR=cypress/install/include/ -DSTATIC_LINKING=FALSE -DSNAB_DEBUG=TRUE
make 
make test ARGS=-V
