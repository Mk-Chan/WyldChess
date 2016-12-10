#!/bin/bash
if [ $# -lt 1 ]
then
	echo "Usage: ./build.sh <release-number>"
	exit
fi
TARGETS=( linux win64 )
for T in "${TARGETS[@]}"
do
	if [ "$T" = "win64" ]
	then
		CC=x86_64-w64-mingw32-gcc
	else
		CC=gcc
	fi
	make clean
	make CC="$CC" RELEASE=$1 TARGET="$T"
	make clean
	make CC="$CC" RELEASE=$1 TARGET="$T" POPCNT=1
	make clean
	make CC="$CC" RELEASE=$1 TARGET="$T" FAST=1
	make clean
	make CC="$CC" RELEASE=$1 TARGET="$T" POPCNT=1 FAST=1
done
make clean
cd binaries
tar cvzf WyldChess_v$1.tar.gz WyldChess_v$1
7z a WyldChess_v$1.zip WyldChess_v$1
