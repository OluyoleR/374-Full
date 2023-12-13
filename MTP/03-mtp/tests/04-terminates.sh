#!/usr/bin/env bash

# Caret test

if [ $# -ne 2 ]
then
  printf 'Usage: %s bin ref_bin\n' "$0"
  exit 1
fi

ref=$1
bin=$2

out=`mktemp -d`
trap "rm -rf $out;" EXIT 
{
  stopline=$((RANDOM % (50 - 10) + 10))
  for lno in {0..49}
  do
    if [ $lno -eq $stopline ]
    then
      break
    else
      shuf -ern $((RANDOM % (1000 - 1) + 1)) x | paste -sd '' 
    fi
  done
  printf 'STOP\n'
  for lno in {1..100}
  do
    shuf -ern $((RANDOM % (1000 - 1) + 1)) x | paste -sd ''
  done
} >"$out/in"

"$ref" 2>/dev/null < <(cat "$out/in") >"$out/reference"
{ timeout -s 9 1 "$bin" < <(cat "$out/in"; sleep 0.5) >"$out/output" ; } &>/dev/null

if [ $? -eq 137 ]
then
  printf 'Program timed out\n'
  exit 1
fi

cmp -l "$out/reference" "$out/output"  1>"$out/cmpout"  2>"$out/cmperr"
ret=$?
if [ -s "$out/cmpout" ]
then
  printf 'Differences detected:\n'
  cat "$out/cmpout" |  awk 'NR > 5 {next} {$2 = strtonum("0" $2); $3 = strtonum("0" $3)} {printf "Byte #%1$d - Reference: " ($2 == 10 ? "`\\n` " : $2 > 32 && $2 < 127 ? "`%2$c`" : "" ) "(%2$d). Output: " ( $3 == 10 ? "`\\n` " : $3 > 32 && $3 < 127 ? "`%3$c` " : "" ) "(%3$d).\n", $1, $2, $3} END {if (FNR > 5) { printf "... and an additional %d differences detected\n", FNR - 5}}'
fi
if [ -s "$out/cmperr" ]
then
  printf 'Additional errors detected:\n'
  sed 's/^cmp:/  /' "$out/cmperr"
fi
exit $ret
