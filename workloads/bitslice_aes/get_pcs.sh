#!/bin/zsh
if [ ! -f workloads/bitslice_aes/test_o.asm ]; then
  echo "test_o.asm file not found in workloads/bitslice_aes" >&2
  exit 1
fi

start_pc=$(rg clflush test_o.asm | sed -e 's|^  ||' -e 's|:.*||' | head -n1)

# rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1
# rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m1
# rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m2
# rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m3
# rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m4
delaying1=$(rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m1 |\
  tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

delaying2=$(rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m2 |\
  tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

delaying3=$(rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m3 |\
  tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

delaying4=$(rg clflush -A50 test_o.asm | rg 'add.*0x2' -A1 -m4 |\
  tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

target=$(rg clflush -A160 test_o.asm | rg 'jmp' -A3 -m1 | tail -n1 |\
  sed -e 's|  ||' -e 's|:.*||' )
# target=$(rg clflush -A160 test_o.asm | rg 'jmp' -A13 -m1 | tail -n1 |\
#   sed -e 's|  ||' -e 's|:.*||' )

post_delaying1=$(rg clflush -A160 test_o.asm | rg 'jmp' -A80 -m1 |\
  rg 'add.*0x2' -A1 -m1 | tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

post_delaying2=$(rg clflush -A160 test_o.asm | rg 'jmp' -A80 -m1 |\
  rg 'add.*0x2' -A1 -m2 | tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

post_delaying3=$(rg clflush -A160 test_o.asm | rg 'jmp' -A80 -m1 |\
  rg 'add.*0x2' -A1 -m3 | tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

post_delaying4=$(rg clflush -A160 test_o.asm | rg 'jmp' -A80 -m1 |\
  rg 'add.*0x2' -A1 -m4 | tail -n 1 | sed -e 's|^  ||' -e 's|:.*||')

echo start: $start_pc
echo delaying1: $delaying1
echo delaying2: $delaying2
echo delaying3: $delaying3
echo delaying4: $delaying4
echo target: $target
echo post_delaying1: $post_delaying1
echo post_delaying2: $post_delaying2
echo post_delaying3: $post_delaying3
echo post_delaying4: $post_delaying4
