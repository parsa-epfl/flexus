# Flexus Instructions

## How to Build Environment

* Install a compatible version of `gcc`:

```sh
$ export GCC_VERSION="8"
$ sudo apt-get update
$ sudo apt-get -y install gcc-${GCC_VERSION} g++-${GCC_VERSION}
```

* Set the recently installed version of `gcc` as default:

```sh
$ sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 20
$ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} 20
$ sudo update-alternatives --config gcc
$ sudo update-alternatives --config g++
```

* Install a compatible version of `boost` libaray:

```sh
$ export BOOST="boost_1_70_0"
$ export BOOST_VERSION="1.70.0"
$ wget https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/${BOOST}.tar.gz -O /tmp/${BOOST}.tar.gz
$ tar -xf /tmp/${BOOST}.tar.gz
$ cd ./${BOOST}/
$ ./bootstrap.sh --prefix=/usr/local
$ ./b2 -j8
$ sudo ./b2 install
```

## How to Use CMake to Compile Flexus

* The default settings are:

```sh
-DTARGET_PLATFORM=arm
-DSELECTED_DEBUG=vverb
-DSIMULATOR=KnottyKraken
-DCMAKE_C_COMPILER=/usr/bin/gcc
-DCMAKE_CXX_COMPILER=/usr/bin/g++
-DBOOST_INCLUDEDIR=/usr/local/include
-DBOOST_LIBRARYDIR=/usr/local/lib
```

* Add options by `-D${OPTION_NAME}=${OPTION}` after `cmake`.

```sh
$ cmake -DSIMULATOR=KnottyKraken . && make -j
```

* Use `make clean` to only remove `*.a` and `*.so` files.
* Use `make clean_cmake` to remove all of the files and folders that cmake produces.

## Flexus Code Formatting

The preferred version of Ubuntu is 18.04. When you contribute to this project, please clang-format the code at first. Please follow these instructions:

```sh
$ sudo apt-get install clang-format-6.0
```

Use the `.clang-format` file from repository and locate it in the folder where you want to format the code.

Within the folder, use this command to format .cpp/.hpp/.c/.h files:

```sh
$ find . -name "*.hpp" -or -name "*.cpp" -or -name "*.h" -or -name "*.c" | xargs clang-format -i
```

Use the `clang_format_test.sh` file to check whether the formatting is successful or not. If it is successful, there will be green works showing `All source code in commit are properly formatted`. Otherwise, there will be red warning showing `Found formatting errors!` and which file where the problem is in.