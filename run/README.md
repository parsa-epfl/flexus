# Preparation

## Install packages

The following packages should be installed to build QEMU and FLEXUS:

- gcc/g++ (with C++14 support) [Which GNU Compiler support which standard](https://gcc.gnu.org/projects/cxx-status.html)
- glib2 (min 2.72)
- boost
- cmake
- meson
- ninja (maybe as a dependency of meson)

No requirements for old/outdated Linux distributions. Just use latest ones.

## Build QEMU

```sh
git clone -b release/8.1-riscv https://github.com/parsa-epfl/qemu
cd qemu
./configure --target-list=riscv64-softmmu
cd build
ninja
```

If you encounter the problem `undefined reference to symbol dlsym@@GLIBC_2.2.5`, please use the following commands to [configure](https://stackoverflow.com/questions/67667369/undefined-reference-to-symbol-dlsymglibc-2-2-5) QEMU:

```sh
./configure --target-list=riscv64-softmmu --extra-ldflags='-Wl,--no-as-needed,-ldl'
```

## Build FLEXUS
```sh
git clone -b riscv https://github.com/parsa-epfl/flexus
cd flexus
./build.sh KeenKraken   # trace simulator
make -C build.KeenKraken
./build.sh KnottyKraken # timing simulator
make -C build.KnottyKraken
```

## Create symlinks
```sh
cd flexus/run
ln -s ${PATH_TO_QEMU}/build/qemu-system-riscv64 qemu
ln -s ../build.KeenKraken/libKeenKraken.so .
ln -s ../build.KnottyKraken/libKnottyKraken.so .
```

# Run

## Normal QEMU

```sh
cd flexus/run
./runq
```

The filesystem now contains basic tools provided by `/bin/busybox` like `ls`, `cd`, etc. The filesystem also contains a `/bin/stress` that can perform basic stress tests.

When you think the simulation proceeds to the point you want, hit Ctrl-A + Ctrl-C to enter the QEMU monitor, and type the following commands:
```sh
(qemu) stop # stops simulation first
(qemu) savevm-external SNAPSHOT_NAME
```

After that you can type `cont` or `c` to continue the simulation or `q` to quit.

You can modify any command line arguments passed to QEMU in the `qemu.cfg` file, which just specifies those arguments in a prettier format.

The `runq` script receives two kinds of parameters. The ones that starts with `+` (e.g., `+dbg`) sets environment variables that change the execution flow of the script itself. Any other arguments are directly passed to qemu as additional arguments. For example, to print CPU execution trace, one can

```sh
./runq -d cpu
```

## Trace simulation

```sh
cd flexus/run
./runq +trace=${SNAPSHOT_NAME}
```

The simulation log is saved in `debug.log` instead of being displayed in the terminal (the latter is broken). You can change parameters passed to FLEXUS in the `trace.cfg` file. When FLEXUS normally exists, it automatically creates a directory `${SNAPSHOT_NAME}/adv` that contains another snapshot for the timing simulation.

## Timing simulation

```sh
cd flexus/run
./runq +timing=${SNAPSHOT_NAME}
```

You can change parameters passed to FLEXUS in the `timing.cfg` file.

## Multicore simulation

To run the simulation with multiple cores, you can just add `+smp=${NR_CPUS}` to the command line of `runq` without changing anything. Note that the same `+smp=` argument should be provided when running QEMU to prepare the shapshot or running trace/timing simulation with that snapshot. For example, to simulate 32 cores:

```sh
cd flexus/run
./runq +smp=32 # then save the snapshot
./runq +smp=32 +timing=${SNAPSHOT_NAME}
```
