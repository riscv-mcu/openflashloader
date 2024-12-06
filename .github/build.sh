#/bin/env bash

# Reset
COLOROFF='\033[0m'       # Text Reset

# Regular Colors
BLACK='\033[0;30m'        # Black
RED='\033[0;31m'          # Red
GREEN='\033[0;32m'        # Green
YELLOW='\033[0;33m'       # Yellow

if [ ! -f $NUCLEI_SDK_ROOT/NMSIS_VERSION ] ; then
    echo -e "${RED}NUCLEI_SDK_ROOT environment variable is not set or path not valid, please set it to where your Nuclei SDK located!${COLOROFF}"
    echo "eg. export NUCLEI_SDK_ROOT=/path/to/nuclei-sdk"
    exit 1
else
    echo -e "${YELLOW}Setup Nuclei SDK build environment${COLOROFF}"
    pushd ${NUCLEI_SDK_ROOT}
    echo -e "${GREEN}Nuclei SDK version is $(git describe --tags --always 2>/dev/null || echo "not a git repo, please check version in ${NUCLEI_SDK_ROOT}/npk.yml")${COLOROFF}"
    source setup.sh
    popd
fi

echo -e "${YELLOW}Here is your NUCLEI_SDK_ROOT=$NUCLEI_SDK_ROOT ${COLOROFF}"

# Exit on error
set -e

echo -e "${YELLOW}Start to do flashloader build test in different build mode${COLOROFF}"
for mode in "sdk" "loader"; do
    for arch in "rv32" "rv64"; do
        for spi in "nuspi" "fespi"; do
            makeflags="MODE=$mode ARCH=$arch SPI=$spi FLASH=w25q256fv"
            echo -e "${YELLOW}Clean and build freeloader for $makeflags ${COLOROFF}"
            make $makeflags clean all
            if [ "$spi" == "fespi" ] && [ "$mode" == "sdk" ]; then
                make $makeflags clean
                echo -e "${YELLOW}Test in qemu and check whether pass or fail${COLOROFF}"
                timeout --foreground -s SIGTERM 30s make $makeflags SIMU=qemu run_qemu
                if [ "x$?" == "x0" ] ; then
                    echo -e "${GREEN}Test in qemu for $makeflags: PASS ${COLOROFF}"
                else
                    echo -e "${RED}Test in qemu for $makeflags: FAIL ${COLOROFF}"
                fi
            fi
            make $makeflags clean
        done
    done
done

echo -e "${GREEN}Freeloader build testing passed!${COLOROFF}"

exit 0
