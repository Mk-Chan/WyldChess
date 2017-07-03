#!/bin/bash

if [ $# -lt 1 ]
then
	echo "Usage: ./build.sh <release-number> [-j<cores>]"
	exit
fi


TARGETS=( linux win64 )

for T in "${TARGETS[@]}"
do
	mkdir -p binaries/WyldChess_v$1/$T
done

cd src

for T in "${TARGETS[@]}"
do
	if [ "$T" = "win64" ]
	then
		CC=x86_64-w64-mingw32-gcc
	else
		CC=gcc
	fi
	make clean
	make CC="$CC" RELEASE=$1 TARGET="$T" $2
	make clean
	make popcnt CC="$CC" RELEASE=$1 TARGET="$T" $2
	make clean
	make bmi CC="$CC" RELEASE=$1 TARGET="$T" $2
done

make clean
cd ../binaries/WyldChess_v$1

BASE=wyldchess

for T in "${TARGETS[@]}"
do
	mv $T "$BASE"_$T
done

for F in wyldchess_win64/*
do
	mv $F $F.exe
done

tar cvzf WyldChess_v$1_linux.tar.gz "$BASE"_linux
7z a WyldChess_v$1_win64.zip "$BASE"_win64
