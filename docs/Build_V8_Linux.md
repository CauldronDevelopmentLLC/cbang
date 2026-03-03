# Bulid V8 for Linux
These instructions are for V8 v10.2.154.13.

# Get Depot tools

    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    export PATH=$PWD/depot_tools:$PATH
    gclient

# Get v8 source

    fetch v8
    cd v8
    git checkout 10.2.154.13
    gclient sync

# Configure release version

    mkdir -p out/x64.release
    cp $CBANG_HOME/docs/v8_args.gn out/x64.release/args.gn
    gn gen out/x64.release

# Build release version

    ninja -C out/x64.release v8_monolith

# Configure debug version

    mkdir -p out/x64.debug
    sed 's/is_debug = false/is_debug = true/' < $CBANG_HOME/docs/v8_args.gn \
        > out/x64.debug/args.gn
    gn gen out/x64.debug

# Build debug version

    ninja -C out/x64.debug v8_monolith

# Point C! to the V8 build

    export V8_INCLUDE=$PWD/include V8_LIBPATH=$PWD/out/x64.release/obj/
