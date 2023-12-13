#!/usr/bin/env bash

if [ $# -ne 1 ]
then
  printf 'Usage: %s\n' "$0"
  exit 1
fi

bin=$1

log() {
  strace-log-merge "$out"/trace 2>&- |
    sed 's/^\([[:digit:]]\+\) \+[[:digit:]]\+:[[:digit:]]\+:[[:digit:]]\+.[[:digit:]]\+ \+\([^(]*\)\((.*)\) *= *\(.*\)/\1 \2 \4 \3/g'
}

readonly out=`mktemp -d`
trap 'rm -rf $out; exec 3>&-; sleep 0.05; kill -9 $proc &>/dev/null; wait' EXIT
mkfifo "$out/stdin"
strace -qqq -ff -v -s0 -tt -o "$out/trace" \
       --status=successful \
       -e trace=clone,futex,read,write,execve \
       "$bin" &>/dev/null <"$out/stdin" &
proc=$!
exec 3>"$out/stdin"
sleep 0.05


procs=($(log | awk 'BEGIN { getline; main=$1; print main; } $1 == main && $2 == "clone" && /CLONE_THREAD/ { print $3; } END { exit 1 }'))
if [ ${#procs[@]} -ne 5  ]
then
  printf 'Detected %d separate threads. Expected 5 (main thread, and four pipeline threads).\n' "${#procs[@]}"
  exit 1
fi
main=${procs[0]}
unset ${procs[0]}

n=$(log | wc -l)
sleep 0.05
if [ "$(log | wc -l)" -ne $n ]
then
  printf 'Detected busy-waiting\n'
  exit 1
fi

printf 'x' >&3
sleep 0.05
k=$(log | wc -l)
if [ $k -gt $((++n)) ]
then
  printf 'Expected one read() call in input thread, for one byte of input. Instead saw %d syscalls\n' $((k-n))
  exit 1
fi
input_thread=$(log | awk '$1 != "'$main'" && $2 == "read" {print $1}')
if [ -z "$input_thread" ]
then
  printf 'Input thread did not read a byte provided on stdin.\n'
  exit 1
fi

printf 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n' >&3
sleep 0.05
if [ $(log | awk '$2 != "futex" { next; } { print $1 }' | sort | uniq | wc -l) -ne 4 ]
then
  printf 'Threads do not appear to be synchronizing data access with mutexes.\n'
  exit 1
fi

if ! $(log | tail -n1 | awk '$1 != "'$main'" && $1 != "'$input_thread'" && $2 == "write" && $3 == "81" { exit 0 ; print $1 } {exit 1}')
then 
  printf 'Output thread is not printing 80 characters plus a newline.\n'
  exit 1
fi
exit 0

