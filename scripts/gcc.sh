#!/usr/bin/env bash

set -e

install_gcc_dependencies() {
    if command -v pacman &>/dev/null; then
        sudo pacman -Syu
        sudo pacman -S base-devel gmp libmpc mpfr
    elif command -v dnf &>/dev/null; then
        sudo dnf update
        sudo dnf install gcc gcc-c++ make bison flex gmp-devel libmpc-devel mpfr-devel texinfo
    elif command -v apt &>/dev/null; then
        sudo apt update && sudo apt upgrade
        sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
    elif command -v apt-get &>/dev/null; then
        sudo apt-get update && sudo apt-get upgrade
        sudo apt-get install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
    elif command -v emerge &>/dev/null; then
        sudo emerge --sync
        sudo emerge --ask dev-build/make sys-devel/bison sys-devel/flex dev-libs/gmp dev-libs/mpfr sys-apps/texinfo
    else
        echo "Error: cannot install gcc dependencies, package manager unknown. Please install the dependencies yourself."
        echo "Dependencies: C/C++ compiler and linker, bison, gmp, mpc, mpfr, flex, texinfo."
        echo "Remember to comment out the line that calls this function, otherwise you will get the same error."
        exit 1
    fi

    # Sudo should not be needed anymore, except when installed to a protected folder.
    # This will just invalidate any previously cached credentials
    sudo -k
}

build_gcc() {
    local GCC_VERSION="$1"
    local BINUTILS_VERSION="$2"
    local TARGET="$3"
    local PREFIX="$4"
    local LIBGCC_CFLAGS="$5"
    local MAKE_FLAGS="$6"
    local PROGRAM_PREFIX="$7"
    
    export PATH="$PREFIX/bin:$PATH"
    mkdir -p "$PREFIX"

    # Create source directories, retrive the source code and then extract it
    mkdir -p cross-compiler
    cd cross-compiler
    curl -O "https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
    curl -O "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz"
    tar xf "binutils-$BINUTILS_VERSION.tar.gz"
    tar xf "gcc-$GCC_VERSION.tar.gz"

    # Configure, build, and install binutils
    mkdir -p binutils-build
    cd binutils-build
    ../binutils-$BINUTILS_VERSION/configure --target="$TARGET" --prefix="$PREFIX" --program-prefix="$PROGRAM_PREFIX" --with-sysroot --disable-nls --disable-werror
    make $MAKE_FLAGS
    make install $MAKE_FLAGS

    cd ..

    # Configure, build, and install gcc and libgcc
    mkdir -p gcc-build
    cd gcc-build
    ../gcc-$GCC_VERSION/configure --target="$TARGET" --prefix="$PREFIX" --program-prefix="$PROGRAM_PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    make all-gcc $MAKE_FLAGS
    make all-target-libgcc CFLAGS_FOR_TARGET="$LIBGCC_CFLAGS"
    make install-gcc $MAKE_FLAGS
    make install-target-libgcc $MAKE_FLAGS

    # Add the compiler to the systems PATH variable in bashrc, create a backup in case anything goes terribly wrong
    cp ~/.bashrc ~/.bashrc.backup
    echo "Created backup ~/.bashrc at ~/.bashrc.backup"
    echo -e "\nexport PATH=\"$PREFIX/bin:\$PATH\"" >> ~/.bashrc

    # Now clean up everything
    cd ../..
    rm -rf ./cross-compiler

    echo "Cross-compiler for $TARGET built successfully!"
}

install_gcc_dependencies # Feel free to comment this line out if you have dependencies installed already

# Feel free to change these options: 
# gcc version (1), binutils version (2), install prefix (4), make flags (6)
# Make sure to make a copy of this script before making any changes
build_gcc "14.1.0" "2.42" "x86_64-elf" "$HOME/.toolchains/gcc/x86_64-crescent" "-mno-red-zone -O2" "-j2" "x86_64-crescent-"
