#!/bin/bash
# ------------------------------------------------------------------------------

PROJECT_HOME="$(cd "$(dirname "${0}")/.." && pwd)"
cd "$PROJECT_HOME"
mkdir -p thirdparty

cd "$PROJECT_HOME/thirdparty"
if [ ! -d "Kuku" ]; then
    echo "-- Clone Git Repository"
    git clone https://github.com/microsoft/Kuku.git
    cd Kuku
    git checkout v2.1.0
    echo "-- Build and Install Kuku library"
    echo "-- Install at $PROJECT_HOME/thirdparty"
    (
        mkdir build
        cd build
        cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX="$PROJECT_HOME/thirdparty" ..
        make
        make install
    )
fi

cd "$PROJECT_HOME/thirdparty"
if [ ! -d "xxHash" ]; then
    echo "-- Clone Git Repository"
    git clone https://github.com/Cyan4973/xxHash.git
    cd xxHash
    git checkout v0.8.2
    echo "-- Build and Install xxHash library"
    echo "-- Install at $PROJECT_HOME/thirdparty"
    (
        mkdir build
        cd build
        cmake -D CMAKE_BUILD_TYPE=Release -D XXHASH_BUILD_XXHSUM=OFF -D CMAKE_INSTALL_PREFIX="$PROJECT_HOME/thirdparty" ../cmake_unofficial
        make
        make install
    )
fi

cd "$PROJECT_HOME/thirdparty"
if [ ! -d "libOTe" ]; then
    echo "-- Clone Git Repository"
    git clone https://github.com/osu-crypto/libOTe.git
    cd libOTe
    git checkout v2.1.0
    echo "-- Build and Install libOTe library"
    echo "-- Install at $PROJECT_HOME/thirdparty"
    (
        python build.py --boost --relic -D ENABLE_MR=ON -D ENABLE_MR_KYBER=OFF -D ENABLE_SILENTOT=ON -D ENABLE_SILENT_VOLE=ON -D COPROTO_ENABLE_BOOST=ON -D ENABLE_BITPOLYMUL=OFF -D ENABLE_KOS=ON -D ENABLE_SOFTSPOKEN_OT=ON -D CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE=false
        python build.py --install="$PROJECT_HOME/thirdparty"
    )
fi
