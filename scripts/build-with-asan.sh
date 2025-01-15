#!/bin/bash -e

if [ "$CBANG_HOME" == "" ]; then
  echo CBANG_HOME not set
  exit 1
fi

CCFLAGS=-fsanitize=address
LDFLAGS="-fsanitize=address -static-libasan"

export SCONS_OPTIONS="ccflags='$CCFLAGS' linkflags='$LDFLAGS' libs=bfd debug=1"

scons -C "$CBANG_HOME" "$@"
