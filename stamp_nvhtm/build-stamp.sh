#!/bin/bash
FOLDERS="bayes genome intruder vacation ssca2 kmeans yada labyrinth"
# FOLDERS="bayes genome intruder vacation kmeans yada labyrinth"
# FOLDERS="vacation kmeans yada"
#FOLDERS="genome"
# FOLDERS="labyrinth"
# FOLDERS="intruder"

if [ $# -eq 0 ] ; then
    echo " === ERROR At the very least, we need the backend name in the first parameter. === "
    exit 1
fi

backend=$1  # e.g.: herwl


htm_retries=5
rot_retries=2

if [ $# -eq 3 ] ; then
    htm_retries=$2 # e.g.: 5
    rot_retries=$3 # e.g.: 2, this can also be retry policy for tle
fi

rm lib/*.o || true

rm Defines.common.mk
rm Makefile
rm Makefile.flags
rm lib/thread.h
rm lib/thread.c
rm lib/tm.h

cp ../$backend/Defines.common.mk .
cp ../$backend/Makefile .
cp ../$backend/Makefile.flags .
cp ../$backend/thread.h lib/
cp ../$backend/thread.c lib/
cp ../$backend/tm.h lib/

if [[ $backend == htm-sgl-nvm ]] ; then
	PATH_NH=../nvm-emulation
	cd $PATH_NH
	make clean && make $MAKEFILE_ARGS
	cd -
fi

if [[ $backend == stm-tinystm ]] ; then
	PATH_TINY=../../../tinystm
	cd $PATH_TINY
	make clean
  make $MAKEFILE_ARGS
	cd -
fi

for F in $FOLDERS
do
  cd $F
  rm *.o || true
  rm $F
  pwd
  make_command="make -j8 -f Makefile $MAKEFILE_ARGS"
  echo " ==========> $make_command"
  $make_command
  rc=$?
  if [[ $rc != 0 ]] ; then
      echo ""
      echo "=================================== ERROR BUILDING $F - $name ===================================="
      echo ""
      exit 1
  fi
  cd ..
done
