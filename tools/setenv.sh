#!/bin/bash

if [ "$#" -ne 1 ]
then
	echo "expect only one argument of which one: m or b, please try again."
	exit -1
fi

# version of corewallet
if [ "$1" = "m" ]; then
	app="ibrio"
	subdir=".ibrio"
	macsubdir="Ibrio"
elif [ "$1" = "b" ]; then
	app="ibrio"
	subdir=".ibrio"
	macsubdir="Ibrio"
else
	echo "there is no parameter like this, please try again."
	exit -1
fi

ostype=`uname`
if [ "$ostype" = "Darwin" ]; then
	dir="$HOME/Library/Application Support/$macsubdir"
else
	dir=$HOME/$subdir
fi

rm -rf "$dir"
mkdir "$dir"
cp ./$app.conf "$dir"
