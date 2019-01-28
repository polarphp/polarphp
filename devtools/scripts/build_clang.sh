#!/bin/bash
currentDir=`pwd`
sourceDir=`dirname $0`
sourceDir=`cd ${sourceDir};pwd`
rootDir=`cd ${sourceDir}/../..;pwd`
if [ ! "${currentDir}" = "${rootDir}" ]
then
    echo "you must run script at root directory of polarphp project"
fi

installDir=/usr/local/clang-7.0
buildVersion=release_70

nproc=4
osname="unknown"
case "$OSTYPE" in
  solaris*) 
    osname=SOLARIS
    ;;
  darwin*)  
    osname=OSX
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

supportedVersions[0]=release_50
supportedVersions[1]=release_60
supportedVersions[2]=release_70

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
tempDir=${rootDir}/temp/clang_build
[ ! -d ${tempDir} ] && mkdir -p ${tempDir}
if [ ! -d ${tempDir} ] 
then
    echo "create ${tempDir} error"
    exit 1
fi

baseRepo=https://github.com/llvm-mirror
llvmZip=${baseRepo}/llvm/archive/${buildVersion}.zip
clangZip=${baseRepo}/clang/archive/${buildVersion}.zip
libcxxZip=${baseRepo}/libcxx/archive/${buildVersion}.zip
libcxxabiZip=${baseRepo}/libcxxabi/archive/${buildVersion}.zip
clangToolsExtraZip=${baseRepo}/clang-tools-extra/archive/${buildVersion}.zip
compilerRtZip=${baseRepo}/compiler-rt/archive/${buildVersion}.zip

[ ! -f ${tempDir}/llvm.zip ] && wget ${llvmZip} -O ${tempDir}/llvm.zip
[ ! -f ${tempDir}/clang.zip ] && wget ${clangZip} -O ${tempDir}/clang.zip
[ ! -f ${tempDir}/libcxx.zip ] && wget ${libcxxZip} -O ${tempDir}/libcxx.zip
[ ! -f ${tempDir}/libcxxabi.zip ] && wget ${libcxxabiZip} -O ${tempDir}/libcxxabi.zip
[ ! -f ${tempDir}/clang-tools-extra.zip ] && wget ${clangToolsExtraZip} -O ${tempDir}/clang-tools-extra.zip
[ ! -f ${tempDir}/compiler-rt.zip ] && wget ${compilerRtZip} -O ${tempDir}/compiler-rt.zip

echo "cd ${tempDir}"
cd ${tempDir} || exit 1
llvmDir=${tempDir}/llvm
if [ ! -d ${llvmDir} ]
then
    echo "unzip llvm.zip ..."
    unzip -q ${tempDir}/llvm.zip || exit 1
    echo "rename llvm directory ..."
    mv llvm-${buildVersion} llvm || exit 1
fi

clangDir=${llvmDir}/tools/clang
if [ ! -d ${clangDir} ]
then
    echo "unzip clang.zip ..."
    unzip -q ${tempDir}/clang.zip || exit 1
    echo "rename clang directory ..."
    mv clang-${buildVersion} ${clangDir} || exit 1
fi

clangToolsExtraDir=${clangDir}/tools/clang-tools-extra
if [ ! -d ${clangToolsExtraDir} ]
then
    echo "unzip clang-tools-extra.zip ..."
    unzip -q ${tempDir}/clang-tools-extra.zip || exit 1
    echo "rename clang-tools-extra directory ..."
    mv clang-tools-extra-${buildVersion} ${clangToolsExtraDir} || exit 1
fi

compilerRtDir=${llvmDir}/projects/compiler-rt
if [ ! -d ${compilerRtDir} ]
then
    echo "unzip compiler-rt.zip ..."
    unzip -q ${tempDir}/compiler-rt.zip || exit 1
    echo "rename compiler-rt directory ..."
    mv compiler-rt-${buildVersion} ${compilerRtDir} || exit 1
fi

libcxxDir=${llvmDir}/projects/libcxx
if [ ! -d ${libcxxDir} ]
then
    echo "unzip libcxx.zip ..."
    unzip -q ${tempDir}/libcxx.zip || exit 1
    echo "rename libcxx directory ..."
    mv libcxx-${buildVersion} ${libcxxDir} || exit 1
fi

libcxxabiDir=${llvmDir}/projects/libcxxabi
if [ ! -d ${libcxxabiDir} ]
then
    echo "unzip libcxxabi.zip ..."
    unzip -q ${tempDir}/libcxxabi.zip || exit 1
    echo "rename libcxxabi directory ..."
    mv libcxxabi-${buildVersion} ${libcxxabiDir} || exit 1
fi

mkdir build
cd build
cmake -Wno-dev -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=${installDir} \
                                   -DLLVM_INCLUDE_TESTS=OFF\
                                   -DLLVM_INCLUDE_EXAMPLES=OFF\
                                   -DLLVM_INCLUDE_BENCHMARKS=OFF\
                                   -DLLVM_ENABLE_EH=ON\
                                   -DLLVM_ENABLE_RTTI=ON\
                                   -DCMAKE_BUILD_TYPE=Release ../llvm
make -j${nproc} && make install