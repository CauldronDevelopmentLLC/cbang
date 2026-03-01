# How to profile code with Linux perf

Notes on how to profile software for speed optimization in Linux with the `perf` program.

## Install

Linux perf can be installed on Debian with:

```
sudo apt-get install -y linux-perf
```

Or you can build a more optimized version by downloading the kernel source.  Download a kernel that matches your system.  `uname -a`.

```
sudo apt-get install -y libdw-dev libelf-dev libtracefs-dev libopencsd-dev \
  libaudit-dev libperl-dev libtraceevent-dev libpfm4-dev libcapstone-dev \
  libbabeltrace-dev llvm-dev systemtap-sdt-dev libdebuginfod-dev

tar xf linux-6.12.74.tar.xz
cd linux-6.12.74
make -j$(nproc)
```

Check the build options with:

```
perf version --build-options
```

## Capture a trace

Build your software in debug mode.  Options `-g3 -fno-inline -fno-omit-frame-pointer -Og` are useful for building properly traceable programs that still run fairly fast.  Just build with ``debug=1`` in C!.

Then while your `PROGRAM` is running:

```
sudo perf record -g --call-graph dwarf -p $(pidof PROGRAM) -- sleep 30
```

## Analyze

This can be done on a different computer if you have the same binary. Copy the ``perf.data`` file to the analysis computer.  You may need to run the following to get the symbols to load:

```
perf buildid-cache --add fah-node
```

Then run the report tool:

```
perf report --addr2line=llvm-symbolizer
```

`llvm-symbolizer` comes from the ``llvm`` package.  Using it *greatly* reduces `perf report`s load time.

Search for entries with large percentages of "self" allocations.  Press `e` to expand rows.