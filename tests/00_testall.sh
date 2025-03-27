#!/bin/bash

source ../make/test-helper.sh

OPERATIONS="bam checkbam dir fat validate"  # showfile read

for OP in $OPERATIONS; do
	for FILE_W_DIR in images/*.d*; do
	#for FILE_W_DIR in images/emp*.d80; do
		IMAGENAME=`basename $FILE_W_DIR`
		if [ ! -z "$IMAGENAME" ]; then
			FILE="${IMAGENAME%.*}"
			EXTENSION="${IMAGENAME##*.}"

			if [ "$IMAGENAME" == "$FILE.$EXTENSION" ]; then
				IN_FILE="$FILE-$EXTENSION-$OP"
				echo TESTING $IN_FILE:
				execute ../output/cbmimage/cbmimage open $FILE_W_DIR $OP
				if [ -e expected-results/$IN_FILE.1 -o -e expected-results/TODO/$IN_FILE.1 ]; then
					check_identical $IN_FILE $OUT_DIR$IN_FILE
				else
					echo "        file expected-results/$IN_FILE.1 does not exist!"
				fi
			else
				echo PROBLEM: $IMAGENAME - $FILE - $EXTENSION
			fi
		fi
	done
done
