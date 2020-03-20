#! bin/sh

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

if [ ! -f "CMakeLists.txt" ]; then
    echo "CMakeLists.txt file not found!"
    exit 1
fi

cmake .

make -j2