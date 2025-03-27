#!/bin/bash

if [ -z "$IN_FILE" ]; then
	IN_FILE=`basename -s .sh $0`
fi

if [ -z "$OUT_DIR" ]; then
	OUT_DIR=../output/testresults/
fi

[ -d $OUT_DIR ] || mkdir -p $OUT_DIR

if [ ! -z "$DO_VALGRIND" ]; then
	VALGRIND="valgrind $VALGRIND_OPT"
else
	VALGRIND=
fi

execute() {
	local OUT_FILE=$OUT_DIR$IN_FILE

	$VALGRIND "$@" > $OUT_FILE.1 2> $OUT_FILE.2
}

check_identical() {
	local IN_FILE_TO_CHECK=expected-results/$1
	local OUT_FILE_TO_CHECK=$2

	local ERROR=0
	local CHECK_ERROR=0

	if [ -e $IN_FILE_TO_CHECK.1 ]; then
		CHECK_ERROR=1
	else
		CHECK_ERROR=0
		if [ -e expected-results/TODO/$1.1 ]; then
			IN_FILE_TO_CHECK=expected-results/TODO/$1
		fi
	fi

	diff -u $IN_FILE_TO_CHECK.1 $OUT_FILE_TO_CHECK.1
	if [ $? -gt 0 ]; then if [ -z "$VALGRIND" ]; then ERROR=1; fi; fi

	diff -u $IN_FILE_TO_CHECK.2 $OUT_FILE_TO_CHECK.2
	if [ $? -gt 0 ]; then if [ -z "$VALGRIND" ]; then ERROR=1; fi; fi

	if [ $ERROR -gt 0 ]; then
		if [ $CHECK_ERROR -gt 0 ]; then
			exit 1
		else
			echo "   ^^^^^^ EXPECTED ERROR, still to be fixed... Continuing."
			echo
		fi
	fi
}

if [ ! -z "$EXEC" ]; then
	execute $EXEC

	check_identical $IN_FILE $OUT_DIR$IN_FILE
fi
