#!/bin/bash

DIR=projs
NAME="htm_alg"
NODE="node30"

find . -name .DS_* -delete

DM=$DIR/$NAME

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

tar -czf $NAME.tar.gz .

ssh $NODE "mkdir $DIR ; mkdir $DM "
scp $NAME.tar.gz $NODE:$DM
ssh $NODE "cd $DM ; gunzip $NAME.tar.gz ; tar -xf $NAME.tar ; \
      rm *.tar *.tar.gz ; ./compile.sh ; "

rm $NAME.tar.gz
