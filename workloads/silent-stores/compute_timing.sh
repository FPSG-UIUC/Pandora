#!/usr/bin/zsh

sl_st_fname=sl_st_pipe_victim/sl_st_pipe_victim.debug.out
sl_st_seqnums=$(rg 'Doing write.*0x2d939' $sl_st_fname | sed -e 's|.*\[sn:||' -e 's|]||')

sl_st_seqnums_arr=("${(@f)$(echo $sl_st_seqnums)}")

types=('init' 'victim' 'correct' 'wrong')
idx=1

for seqnum in $sl_st_seqnums_arr; do
  # echo "\n$seqnum"
  echo $types[idx]
  idx=$(($idx + 1))
  if [ $idx -eq 5 ]; then
    idx=2
  fi

  o3pipe=$(rg -A 6 "O3PipeView:fetch.*$seqnum:" $sl_st_fname)
  # echo "$o3pipe"
  echo "-----------------"
  fetch=$(echo $o3pipe | head -n 1 | sed -e 's|.*fetch:||' -e 's|:.*||')
  complete=$(echo $o3pipe | tail -n 2 | head -n 1 | sed -e 's|.*complete:||')
  retire=$(echo $o3pipe | tail -n 1 | sed -e 's|.*retire:||' -e 's|:.*||')
  store=$(echo $o3pipe | tail -n 1 | sed -e 's|.*store:||')

  # echo $fetch
  # echo $complete
  # echo $retire
  # echo $store

  echo "Total Time: $(($store - $fetch))"
  echo "Complete--Retire $(($retire - $complete))"
  echo "Retire--Store $(($store - $retire))\n"
done
