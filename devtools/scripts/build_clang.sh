#!/bin/bash
currentDir=`pwd`
sourceDir=`dirname $0`
sourceDir=`cd ${sourceDir};pwd`
rootDir=`cd ${sourceDir}/../..;pwd`
if [ ! "${currentDir}" = "${rootDir}" ]
then
    echo "you must run script at root directory of polarphp project"
fi
installDir=/usr/local
buildVersion=release_70
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
    exist 1
fi

baseRepo=https://github.com/llvm-mirror
llvmRepo=${baseRepo}/llvm/archive/release_70.zip
clangRepo=${baseRepo}/clang/archive/release_70.zip
libcxxRepo=${baseRepo}/libcxx/archive/release_70.zip
clangToolsExtra=${baseRepo}/clang-tools-extra/archive/release_70.zip
compilerRt=${baseRepo}/compiler-rt/archive/release_70.zip

[ ! -f ${llvmRepo} ] && wget ${llvmRepo} -O ${tempDir}/llvm.zip
[ ! -f ${clangRepo} ] && wget ${clangRepo} -O ${tempDir}/clang.zip
[ ! -f ${libcxxRepo} ] && wget ${libcxxRepo} -O ${tempDir}/llcxx.zip
[ ! -f ${clangToolsExtra} ] && wget ${clangToolsExtra} -O ${tempDir}/clang-tools-extra.zip
[ ! -f ${compilerRt} ] && wget ${compilerRt} -O ${tempDir}/compiler-rt.zip

llvmDir=${tempDir}/llvm
if [ ! -d ${llvmDir} ]
then
# create compile structure
    
fi

# if [ ! -d ${clangDir} ]
# then
#     clangDir=${llvmDir}/tools/clang
#     git clone --depth 1 -b ${buildVersion} ${clangRepo} ${llvmDir}/tools
#     if [ ! -d ${clangDir} ]
#     then
#         echo "clone clang repo error"
#         exit 1
#     fi
# fi 

# if [ ! -d ${clangToolsExtra} ]
# then
#     clangToolsExtra=${clangDir}/tools/clang-tools-extra
#     git clone --depth 1 -b ${buildVersion} ${clangToolsExtra} ${clangDir}/tools
#     if [ ! -d ${clangToolsExtra} ]
#     then
#         echo "clone clang-tools-extra repo error"
#         exit 1
#     fi
# fi

# if [ ! -d ${compilerRtDir} ]
# then
#     compilerRtDir=${llvmDir}/projects/compiler-rt
#     git clone --depth 1 -b ${buildVersion} ${compilerRt} ${llvmDir}/projects
#     if [ ! -d ${compilerRtDir} ]
#     then
#         echo "clone compiler-rt repo error"
#         exit 1
#     fi
# fi

# if [ ! -d ${libcxxDir} ]
# then 
#     libcxxDir=${llvmDir}/projects/libcxx
#     git clone --depth 1 -b ${buildVersion} ${libcxxRepo} ${llvmDir}/projects
#     if [ ! -d ${libcxxDir} ]
#     then
#         echo "clone libcxx repo error"
#         exit 1
#     fi
# fi
