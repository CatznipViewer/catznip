#!/bin/bash
FILES="$1/*.patch"
for f in $FILES
do
  echo "Processing $f..."
  git apply "$f" --ignore-whitespace
done
