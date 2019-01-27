#!/bin/bash

currentDir=`pwd`
sourceDir=`dirname $0`
sourceDir=`cd ${sourceDir};pwd`
rootDir=`cd ${sourceDir}/../..;pwd`
dockerFileDir=${rootDir}/devtools/docker

if [ ! "${currentDir}" = "${rootDir}" ]
then
    echo "you must run script at root directory of polarphp project"
fi

isDebugBuild=yes

# if we don't specify version, we build newest version
while (( "$#" )); do
  case "$1" in
    --RELEASE_BUILD)
      isDebugBuild=no
      shift 1
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

buildDir=build/polarphp
if [ "${isDebugBuild}" = "yes" ]
then
    buildDir="${buildDir}/debug"
else
    buildDir="${buildDir}/release"
fi

if [ ! -d ${buildDir} ]
then
    mkdir -p ${buildDir} || exit 1
fi
cd ${buildDir} || exit 1

polarphpBaseEnv=polarphp_base_env

echo "copying docker files ..."

cp ${dockerFileDir}/debug_image_dockerfile debug_image_dockerfile
cp ${dockerFileDir}/release_image_dockerfile release_image_dockerfile
cp ${dockerFileDir}/polarphp_base_image_dockerfile polarphp_base_image_dockerfile

# check base docker image exist
echo "checking polarphp base image ... "
hasBaseEnv=`docker images | grep ${polarphpBaseEnv}`
if [ -z "$hasBaseEnv" ]
then
    docker build -t ${polarphpBaseEnv}:latest --file polarphp_base_image_dockerfile .
fi

echo "prepare polarphp source files ... "
if [ ! -d polarphp ]
then
    mkdir polarphp || exit
else
    rm -Rf polarphp/*
fi

mkdir polarphp/assets || exit 1
cp -Rf ${rootDir}/include polarphp/ || exit 1
cp -Rf ${rootDir}/src polarphp/ || exit 1
cp -Rf ${rootDir}/stdlib polarphp/ || exit 1
cp -Rf ${rootDir}/cmake polarphp/ || exit 1
cp -Rf ${rootDir}/thirdparty polarphp/ || exit 1
cp -Rf ${rootDir}/tools polarphp || exit 1
cp -Rf ${rootDir}/artifacts polarphp/ || exit 1
cp -Rf ${rootDir}/CMakeLists.txt ${rootDir}/configure.ac polarphp/ || exit 1
cp -Rf ${rootDir}/assets/CMakeLists.txt ${rootDir}/assets/php.ini-development ${rootDir}/assets/php.ini-production polarphp/assets || exit 1

