#!/bin/bash

currentDir=`pwd`
sourceDir=`dirname $0`
sourceDir=`cd ${sourceDir};pwd`
rootDir=`cd ${sourceDir}/../..;pwd`

CC=gcc
CXX=g++

if [ ! "${currentDir}" = "${rootDir}" ]
then
    echo "you must run script at root directory of polarphp project"
fi

nproc=4
osname="unknown"
case "$OSTYPE" in
  solaris*) 
    osname=SOLARIS
    ;;
  darwin*)  
    osname=OSX
    CC=clang
    CXX=clang++
    nproc=`sysctl -n hw.ncpu`
    ;; 
  linux*)   
    osname=LINUX
    ;;
  bsd*)
    osname=BSD
    ;;
  msys*)
    osname=WINDOWS
    ;;
  *)       
    echo "unknown: $OSTYPE"
    osname=${OSTYPE}
    ;;
esac

export CC
export CXX

installDir=/usr/local/gcc-8.2.0
buildVersion=gcc-8.2.0

# if we don't specify version, we build newest version
while (( "$#" )); do
  case "$1" in
    --INSTALL_DIR)
      installDir=$2
      shift 2
      ;;
    --TARGET_VERSION)
      buildVersion=$2
      shift 2
      ;; 
    -*|--*=) # unsupported flags
      echo "Error: Unsupported flag $1" >&2
      exit 1
      ;;
    *) # preserve positional arguments
      PARAMS="$PARAMS $1"
      shift
      ;;
  esac
done

# calculate position args
eval set -- "$PARAMS"

supportedVersions[0]=gcc-8.2.0

support=no
for v in ${supportedVersions[*]}
do
    if [ "${v}" = "${buildVersion}" ]
    then
        support=yes
    fi
done
if [ "${support}" = "no" ]
then
    echo "target version is not support"
    exit 1
fi

if [ ! -d ${installDir} ]
then
    echo "install dir ${installDir} is not exist"
    exit 1
fi

# create build temp dir
tempDir=${rootDir}/temp/gcc_build
[ ! -d ${tempDir} ] && mkdir -p ${tempDir}
if [ ! -d ${tempDir} ] 
then
    echo "create ${tempDir} error"
    exit 1
fi

baseRepo=http://ftp.tsukuba.wide.ad.jp/software/gcc
gccTar=${baseRepo}/releases/${buildVersion}/${buildVersion}.tar.gz
gmpTar=${baseRepo}/infrastructure/gmp-6.1.0.tar.bz2
mpfrTar=${baseRepo}/infrastructure/mpfr-3.1.4.tar.bz2
mpcTar=${baseRepo}/infrastructure/mpc-1.0.3.tar.gz
islTar=${baseRepo}/infrastructure/isl-0.18.tar.bz2

echo "cd ${tempDir}"
cd ${tempDir} || exit 1
echo "retrieve gcc release files ... "
[ ! -f ${buildVersion}.tar.gz ] && wget ${gccTar}
[ ! -f gmp-6.1.0.tar.bz2 ] && wget ${gmpTar}
[ ! -f mpfr-3.1.4.tar.bz2 ] && wget ${mpfrTar}
[ ! -f mpc-1.0.3.tar.gz ] && wget ${mpcTar}
[ ! -f isl-0.18.tar.bz2 ] && wget ${islTar}

echo "build compile directory ... "
[ ! -d ${buildVersion} ] && tar zxf ${buildVersion}.tar.gz
[ ! -f ${buildVersion}/gmp-6.1.0.tar.bz2 ] && cp gmp-6.1.0.tar.bz2 ${buildVersion}
[ ! -f ${buildVersion}/mpfr-3.1.4.tar.bz2 ] && cp mpfr-3.1.4.tar.bz2 ${buildVersion}
[ ! -f ${buildVersion}/mpc-1.0.3.tar.gz ] && cp mpc-1.0.3.tar.gz ${buildVersion}
[ ! -f ${buildVersion}/isl-0.18.tar.bz2 ] && cp isl-0.18.tar.bz2 ${buildVersion}

cd ${buildVersion} || exit 1

contrib/download_prerequisites && ./configure\
                            --prefix=${installDir}\
                            --disable-bootstrap\
                            --disable-multilib\
                            --enable-languages=c,c++
make -j$nproc && make install