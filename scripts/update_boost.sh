#!/bin/bash -e

################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2024, Cauldron Development  Oy              #
#               Copyright (c) 2003-2021, Cauldron Development LLC              #
#                              All rights reserved.                            #
#                                                                              #
#        The C! library is free software: you can redistribute it and/or       #
#       modify it under the terms of the GNU Lesser General Public License     #
#      as published by the Free Software Foundation, either version 2.1 of     #
#              the License, or (at your option) any later version.             #
#                                                                              #
#       The C! library is distributed in the hope that it will be useful,      #
#         but WITHOUT ANY WARRANTY; without even the implied warranty of       #
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      #
#                Lesser General Public License for more details.               #
#                                                                              #
#        You should have received a copy of the GNU Lesser General Public      #
#                License along with the C! library.  If not, see               #
#                        <http://www.gnu.org/licenses/>.                       #
#                                                                              #
#       In addition, BSD licensing may be granted on a case by case basis      #
#       by written permission from at least one of the copyright holders.      #
#          You may request written permission by emailing the authors.         #
#                                                                              #
#                 For information regarding this software email:               #
#                                Joseph Coffland                               #
#                         joseph@cauldrondevelopment.com                       #
#                                                                              #
################################################################################

VERSION=1.83.0
NAME=boost_$(echo $VERSION | tr . _)
PKG=$NAME.7z
URL=https://boostorg.jfrog.io/artifactory/main/release/$VERSION/source/$PKG
BCP=./$NAME/dist/bin/bcp

# Get boost
if [ ! -d $NAME ]; then
    if [ ! -e $PKG ]; then wget $URL; fi
    7z x -y $PKG
fi

# Build bcp
if [ ! -e $BCP ]; then
    pushd $NAME
    ./bootstrap.sh
    ./b2 tools/bcp
    popd
fi

# Extract files
$BCP --scan --boost=$NAME --unix-lines $(
    find src/cbang -name \*.h -o -name \*.cpp
) src/boost

# Remove uneeded files
for PATH in Jamroot libs/config libs/date_time libs/smart_ptr \
                    $(find src/boost/boost -name edg -o -name dmc \
                           -o -name no_ctps -o -name no_ttp -o -name mwcw \
                           -o -name msvc70 -o -name msvc60 -o -name bcc_pre590 \
                           -o -name bcc551 -o -name bcc) \
                    "libs/iostreams/src/lzma.cpp" \
                    "libs/iostreams/src/zstd.*"; do
    /bin/rm -rf src/boost/$PATH
done

# Done
echo ok
