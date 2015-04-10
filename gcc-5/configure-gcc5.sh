#!/bin/bash -l

################################################################################
# Download, configure and install script for the experimental gcc 5.           #
#                                                                              #
# This script is meant to be a quick and dirty guide to download, configure    #
# and install the experimental svn trunk gcc-5.0 with offload capabilities.    #
#                                                                              #
# This script was developed within the INFN COSA project:                      #
# http://www.cosa-project.it/                                                  #
#                                                                              #
# For any question, please ask to: enrico.calore@fe.infn.it                    #
#                                                                              #
################################################################################

# exit script if a commend fail
set -e 
set -o pipefail

################################################################################
## Editable settings                                                          ##
################################################################################

SCRIPTDIR=$PWD
# Source code dir
SRCDIR=svn-trunk
# Build dir for host compiler
BUILDDIR=gcc-build
# Build dir for MIC compiler
OBJDIRMIC=gcc-build-mic
# Build dir for GPU compiler
OBJDIRPTX=gcc-build-ptx
# Installation directory (leave empty to install in prefix)
INSTALLDIR=$SCRIPTDIR/night-build
# Temp dirs for nvptx tools and newlib
NVPTXNEWLIBDIR=$SCRIPTDIR/nvptx-newlib
NVPTXTOOLSDIR=$SCRIPTDIR/nvptx-tools
# Available number of cores to compile gcc
MAKE_PROCS=32

cd $SCRIPTDIR

################################################################################

# Uncomment the following line if you use environment modules
module load cuda/6.5

# Otherwise uncomment the following block if you want to manually set env vars
#CUDAROOT=/opt/nvidia/cuda-6.5
#export PATH=$CUDAROOT/bin:$PATH
#export INCLUDE=$CUDAROOT/include:$INCLUDE
#export LD_LIBRARY_PATH=$CUDAROOT/lib:$CUDAROOT/lib64:/usr/lib:/usr/lib64:$LD_LIBRARY_PATH



################################################################################



# Uncomment the following block if you are running in an empty directory
#echo "Checking out latest gcc svn revision..."
#svn checkout svn://gcc.gnu.org/svn/gcc/trunk $SRCDIR
#cd $SRCDIR
#./contrib/download_prerequisites
#echo "Checking out latest nvptx-tools git revision..."
#cd ..

# Otherwise, uncomment the following block
# if you previously checked out svn://gcc.gnu.org/svn/gcc/trunk in $SRCDIR
echo "Updating gcc-5 to latest svn revision..."
cd $SRCDIR
svn update
./contrib/download_prerequisites
cd ..
# Remove old build dirs...
rm -rf $BUILDDIR $OBJDIRMIC $OBJDIRPTX



################################################################################
## You should not need to edit under this line                                ##
################################################################################

rm -rf $NVPTXNEWLIBDIR $NVPTXTOOLSDIR
git clone https://github.com/MentorEmbedded/nvptx-newlib.git
git clone https://github.com/MentorEmbedded/nvptx-tools.git

cd $NVPTXNEWLIBDIR
./configure
make
cd ..
cp -fa nvptx-newlib $SRCDIR/gcc/newlib

cd $NVPTXTOOLSDIR
./configure --with-cuda-driver=$CUDAROOT --prefix=$NVPTXTOOLSDIR
make
make install
cd ..

## MIC Accelerator ##
echo "Configuring Intel MIC compiler..."
mkdir $OBJDIRMIC
cd $OBJDIRMIC
../$SRCDIR/configure --enable-threads=posix --disable-multilib --enable-languages=c,c++ --build=x86_64-intelmicemul-linux-gnu --host=x86_64-intelmicemul-linux-gnu --target=x86_64-intelmicemul-linux-gnu --enable-as-accelerator-for=x86_64-unknown-linux-gnu

echo "Building Intel MIC compiler..."
make -j $MAKE_PROCS

echo "Installing Intel MIC compiler..."
make install DESTDIR=$INSTALLDIR

cd ..

## NVIDIA Accelerator ##
echo "Configuring nVIDIA GPU compiler..."
mkdir $OBJDIRPTX
cd $OBJDIRPTX
../$SRCDIR/configure --target=nvptx-none --enable-as-accelerator-for=x86_64-unknown-linux-gnu --with-build-time-tools=$NVPTXTOOLSDIR/nvptx-none/bin --disable-sjlj-exceptions --enable-newlib-io-long-long --enable-languages=c,c++

echo "Building with nvptx support..."
make -j $MAKE_PROCS

echo "Installing nVIDIA GPU compiler..."
make install DESTDIR=$INSTALLDIR

cd ..

## Configuring Host Compiler ##
echo "Configuring Host compiler with offload targets..."
mkdir $BUILDDIR
cd $BUILDDIR
../$SRCDIR/configure --enable-threads=posix --disable-multilib --enable-languages=c,c++ --build=x86_64-unknown-linux-gnu --host=x86_64-unknown-linux-gnu --target=x86_64-unknown-linux-gnu --enable-offload-targets=x86_64-intelmicemul-linux-gnu=$INSTALLDIR/usr/local,nvptx-none=$INSTALLDIR/usr/local

echo "Building host compiler..."
make -j $MAKE_PROCS

echo "Installing Host compiler..."
make install DESTDIR=$INSTALLDIR


## Print some hints to set vars
echo "Installation completed... "
echo "Set the following variables to be able to use the newly installed gcc-5:"
echo " "
echo "export LD_LIBRARY_PATH=$INSTALLDIR/usr/local/lib:$INSTALLDIR/usr/local/lib64:$LD_LIBRARY_PATH"
echo "export PATH=$INSTALLDIR/usr/local/bin:$PATH"
echo "export INCLUDE=$INSTALLDIR/usr/local/include:$INCLUDE"

