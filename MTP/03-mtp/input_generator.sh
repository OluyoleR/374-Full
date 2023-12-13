chars=(\  \! \" \# \$ \% \& \' \( \) \* \+ \, \- \. \/ {0..9} \: \; \< \= \> \? \@ {A..z} \{ \| \} \~)
stopline=$((RANDOM % (50 - 10) + 10))
for lno in $(seq $((RANDOM % 20 + stopline + 100)))
do
  if [ $lno -eq $stopline ]
  then
    printf 'STOP\n'
  else
    shuf -ern $((RANDOM % (1000 - 1) + 1)) ++{,,,,} ^{,,,,} "${chars[@]}" | paste -sd '' | cut -c1-999
  fi
done

