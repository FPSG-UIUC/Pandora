#!/usr/bin/zsh

cp_file=$1; shift

err_out=$1; shift

function send2slack() {
  curl -X POST -H 'Content-type: application/json' --data "{\"text\":\"$@\"}" https://hooks.slack.com/services/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 2> /dev/null
}

outdir=$1; shift
: ${gem5_outpath=/scratch/pshome2/gem5_out}
: ${gem5_tarpath=/shared/pshome2/gem5_out}

echo 'cp_file: ' $cp_file
echo 'cp_file:t: ' "$cp_file:t"
echo 'err_out: ' $err_out
echo 'outdir: ' $outdir
echo 'outpath: ' $gem5_outpath
echo 'tarpath: ' $gem5_tarpath

echo "$@"

export M5_OVERRIDE_PY_SOURCE=true

# run!
time $@ || send2slack "Error in $outdir:\n\`\`\`$(head -n 20 $err_out)\`\`\`"

mkdir -p $gem5_outpath/$outdir
mv $cp_file  "$gem5_outpath"/"$outdir"/"$cp_file:t"
cd $gem5_outpath/$outdir
rename "s/^/$outdir./" *

source /home/josers2/anaconda3/etc/profile.d/conda.sh
conda activate slst
/shared/jose/purge/silent_stores_binaries/bitslice_aes/vis_timing.py "$outdir"."$cp_file" $gem5_outpath/$outdir

cd ..
time tar czf $outdir.tar.gz $outdir
mv $outdir.tar.gz $gem5_tarpath
rm -r $outdir

send2slack "$outdir finished"
# export SLACK_CLI_TOKEN="xoxb-46412655301-1236295230642-fDfS0Cl5cRVUNaFb5qjzaES6"
# $HOME/slack chat send "PURGE workload completed." "UNBJ59T0C"


