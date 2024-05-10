C! (a.k.a. C-Bang)
==================

The C! or cbang library is a collection of C++ utility libraries
developed over the course of +20 years and several major C++
application development projects.  It should compile and run on
Windows, Linux and macOS using a modern C++ compiler such as GNU C++,
clang or MSVC.

Many of the facilities of C! are geared towards cross-platform
application development and providing basic services that most
applications need such as a configuration system, run-time build
information, configuration and logging facilities, and smart pointers.

C!'s philosophy is to create clean, simple, readable, modular and
reusable code.  C! also encourages exception based error handling,
light use of C++ templates and C preprocesor macros.  Event driven
asynchronoius programing is prefered but threading is also fully
supported.

C! "leans" on the venerable boost library but also reimplements
several boost APIs which are considered by the author to be too
template heavy, less readable or overly complicated in boost.

The code was developed on an as needed basis and was never intended to
be any sort of grand unifying system for C++ application development.
However, I hope you find many parts of the library useful in your C++
development projects.

For licensing information please see ``LICENSE`` file.

# Features

  - Smart pointers, both thread safe and non, with downcasting capability.
  - Extensively configurable logging facilities.
  - Stack trace capability with function, file and line. (macOS and Linux only)
  - Exception throwing and catching convenience macros.
  - Command line parsing and configuration file configuation system.
  - HTTP, HTTPS and Websocket client and server.
  - Automatic ACMEv2 TLS certification aquisition.
  - OAuth2 logins.
  - Event driven programming via C++ wrapper for libevent.
  - Asynchronous IP address / hostname resolution, parsing and manipulation.
  - IPv6 support.
  - Socket client and server with asynchronous capability.
  - Threading, mutexes, conditions & thread local storage
  - Soft exit & signal handling
  - GPU detection with OpenCL and CUDA support detection.
  - CPU type and feature detection for x86, amd64 and aarch64.
  - PCI bus enumeration.
  - Embedded Javascript (ECMA) support.
  - Dynamic library and symbol loading.
  - Subprocess handling.
  - Powermanagment and system idle detection.
  - XMacro based enumerations with automatic string parsing and printing.
  - Software build information system.
  - Compile in resource trees from directory of files on disk.
  - Network packet automatic byte order and string packing functionality.
  - Temporary directories, directory traversal & file operations.
  - File locking.
  - C++ style number conversions to/from string.
  - std::iostream and compression utilities.
  - JSON, YAML and XML facilities.
  - TAR file read and write.
  - Time and timing functions.
  - OpenSSL C++ interface.
  - URL parsing.
  - Geometric primitives.
  - Async MariaDB/MySQL client interface.
  - Human readable size & time formatting.
  - Many other utility classes.
  - Completely contained in it's own C++ namespace.
  - Consistent and clean code formating <= 80 column width.
  - Platform independent API.

# Prerequisites
## Required
  - A modern C++ compiler: GNU C++, Intel C++, MSVS
  - SCons http://scons.org/

## Optional
The following add optional features to C!.

  - openssl    http://www.openssl.org/
  - V8         https://developers.google.com/v8/
  - mariadb    https://mariadb.org/
  - LevelDB    https://github.com/google/leveldb
  - Snappy     http://google.github.io/snappy/

## Debug mode
Debug mode is enabled by compiling with ``debug=``.

Compiling with ``libbfd`` will enhance stack track output on Linux systems.
Install ``binutils-dev`` to get ``libbfd`` on Debian.

## Debian/Ubuntu install
On most Debian based systems the prerequisites can be installed with the
following command line:

    sudo apt-get install -y scons build-essential libssl-dev binutils-dev \
      libiberty-dev libmariadb-dev-compat libleveldb-dev libsnappy-dev git \
      libzstd-dev

The necessary library packages can very by Linux distribution type and version.

# Build

    cd cbang
    scons

There are a number of build options which can either be passed on the
command line to scons or place in a configuration file named
'options.py'.  These options are as follows:

  - debug                - Enable debugging
  - optimize             - Enable compiler optimizations
  - staticlib            - Build a static library
  - sharedlib            - Build a shared library
  - strict               - Enable strict checking
  - distcc               - Enable distcc distributed compile system
  - ccache               - Enable ccache caching compile system
  - backtrace_debugger   - Enable the printing of stack traces

These options are enabled by setting them to 1.  For example:

    scons debug=1

## Build Warnings/Errors
If you get any build warnings, by default, the build will stop.  If you have
problems building, especially with warnings related to the boost library you
can ignore these warnings by building with `scons strict=0`.  This disables
strict checking.

# Testing the Build

You can testing C! by running the test suite:

    scons test
