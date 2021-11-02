#!/bin/bash

FILES=`ls -d -1 $1*`

numCorrect=0
total=0

for f in $FILES; do
  echo "processing: $f"
  isCorrect=`./alive-tv -backend-tv --disable-undef-input "$f" |& grep -w "Transformation seems"`
  if [ -n "$isCorrect" ]; then
    numCorrect=$((numCorrect+1))
  fi
  total=$((total+1))
done

printf "Number correct: %d\n" "$numCorrect"
echo "Percentage correct: $(echo "($numCorrect/$total)*100" | bc -l)"
