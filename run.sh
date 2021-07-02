cd workloads/bitslice_aes
make
cd ../..

mkdir -p results

./simulator/build/X86/gem5.opt \
  --outdir=results \
  --debug_flags=SilentStores,O3PipeView,Fetch,Cache,LSQUnit,ROB \
  --debug-file=debug.out \
  simulator/configs/example/se.py \
  --cmd=workloads/bitslice_aes/test.o \
  -o 100 \
  --mem-type=DDR4_2400_8x8 \
  --cpu-type=DerivO3CPU \
  --l1d_assoc=4 \
  --l1d_size=8kB \
  --caches
