#!/usr/bin/env bash

# If any commands fail, the script won't continue
set -e

# These versions are not required, you can use older or newer versions
GCC_VERSION="13.1.0"
BINUTILS_VERSION="2.40"

# The prefix can be anything you want
TARGET="i686-elf"
PREFIX="$HOME/.toolchains/gcc/$TARGET"
export PATH="$PREFIX/bin:$PATH"

# Any flags the user wants to put in, like -j or something like that
MAKE_FLAGS="$@"
# CFLAGS for libgcc
LIBGCC_CFLAGS="-O2"

mkdir -p $PREFIX

# Get binutils and gcc
mkdir cross-compiler
cd cross-compiler
curl -O https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz
curl -O https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz

tar xf binutils-$BINUTILS_VERSION.tar.gz
tar xf gcc-$GCC_VERSION.tar.gz

mkdir binutils-build
mkdir gcc-build

# Build binutils
cd binutils-build
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make $MAKE_FLAGS
make install $MAKE_FLAGS

# Build gcc and libgcc
cd ../gcc-build
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc $MAKE_FLAGS
make all-target-libgcc $MAKE_FLAGS CFLAGS_FOR_TARGET="$LIBGCC_CFLAGS"
make install-gcc $MAKE_FLAGS
make install-target-libgcc $MAKE_FLAGS

# Just in case something goes wrong here, you can restore it from the backup
cp ~/.bashrc ~/.bashrc.backup
echo "Created backup ~/.bashrc at ~/.bashrc.backup"
echo -e "\nexport PATH=\"$PREFIX/bin:\$PATH\"" >> ~/.bashrc

cd ../..
rm -rf ./cross-compiler

