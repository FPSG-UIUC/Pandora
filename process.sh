#!/bin/zsh

cd workloads/bitslice_aes/

source get_pcs.sh

debugf=$1
fpath=${debugf%/*}
picklef="$fpath"/rois.p

echo $debugf
echo $fpath
echo $picklef

if [ $# -ne 2 ]; then

    # -e 'O3PipeView:fetch.*0x0040103f' \
    # -e 'O3PipeView:fetch.*0x0040118f' \
  rg -A6 $debugf \
    -e 'O3PipeView:fetch.*0xbeef' \
    -e 'O3PipeView:fetch.*0xdead' \
    -e 'O3PipeView:fetch.*0xfeed' \
    -e 'O3PipeView:fetch.*0xbead' \
    -e 'O3PipeView:fetch.*clflush' \
    -e "O3PipeView:fetch.*0x00$delaying1" \
    -e "O3PipeView:fetch.*0x00$delaying2" \
    -e "O3PipeView:fetch.*0x00$delaying3" \
    -e "O3PipeView:fetch.*0x00$delaying4" \
    -e "O3PipeView:fetch.*0x00$post_delaying1" \
    -e "O3PipeView:fetch.*0x00$post_delaying2" \
    -e "O3PipeView:fetch.*0x00$post_delaying3" \
    -e "O3PipeView:fetch.*0x00$post_delaying4" \
    -e "O3PipeView:fetch.*0x00$target" \
    | python inst_parse.py -m 6 $picklef "0x00$target"

  start=0
  roif=icorr_roi.debug.out

  python get_ranges.py $picklef "0x00$start_pc" '0x004012dd' -s

  for tick in $(python get_ranges.py $picklef "0x00$start_pc" '0x004012dd' -t -s); do
    line=$(rg $tick -m 1 $debugf -n | sed -e 's|:.*||')
    echo $tick "->" $line
    if [ $start -eq 0 ]; then
      start=$line
    else
      end=$line
      echo $start -- $end
      tail -n +"$start" $debugf | head -n $(($end - $start)) > "$fpath"/"$roif"
      start=0
      roif=corr_roi.debug.out
    fi
  done

  rg -e 'O3PipeView:fetch' -A6 \
    "$fpath"/corr_roi.debug.out "$fpath"/icorr_roi.debug.out -I \
    | python inst_parse.py "$fpath"/roi_insts.p "0x00$target"

fi

if [ $# -eq 2 ] && [ "$2" = "-v" ]; then
    rg "$fpath"/icorr_roi.debug.out "$fpath"/corr_roi.debug.out -e 'SQ size:' \
      -e 'rob.*Now has' | python plot_events.py "$fpath"/roi_insts.p -v \
      "0x00$target" "0x00$delaying1" "0x00$delaying2" "0x00$delaying3" \
          "0x00$delaying4" "0x00$post_delaying1" "0x00$post_delaying2" \
              "0x00$post_delaying3" "0x00$post_delaying4"
else
    rg "$fpath"/icorr_roi.debug.out "$fpath"/corr_roi.debug.out -e 'SQ size:' \
      -e 'rob.*Now has' | python plot_events.py "$fpath"/roi_insts.p -v -p \
      "$fpath"/plot.pdf "0x00$target" "0x00$delaying1" "0x00$delaying2" \
      "0x00$delaying3" "0x00$delaying4" "0x00$post_delaying1" \
          "0x00$post_delaying2" "0x00$post_delaying3" "0x00$post_delaying4"
fi

