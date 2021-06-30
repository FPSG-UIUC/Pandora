let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~/projects/silent_stores_poc
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
badd +704 src/cpu/o3/lsq.hh
badd +1 src/cpu/o3/lsq.cc
badd +1 src/cpu/o3/lsq_impl.hh
badd +1 src/cpu/o3/lsq_unit.cc
badd +388 src/cpu/o3/lsq_unit.hh
badd +1457 src/cpu/o3/lsq_unit_impl.hh
badd +1197 src/cpu/o3/commit_impl.hh
badd +129 src/cpu/base_dyn_inst_impl.hh
badd +1115 src/cpu/o3/fetch_impl.hh
badd +124 src/cpu/static_inst.cc
badd +273 src/arch/x86/insts/static_inst.cc
badd +747 src/arch/x86/process.cc
badd +2 ~/projects/silent_stores_poc
badd +134 src/base/cprintf.hh
badd +113 src/arch/x86/insts/microop.hh
argglobal
%argdel
$argadd src/cpu/o3/lsq.hh
$argadd src/cpu/o3/lsq.cc
$argadd src/cpu/o3/lsq_impl.hh
$argadd src/cpu/o3/lsq_unit.cc
$argadd src/cpu/o3/lsq_unit.hh
$argadd src/cpu/o3/lsq_unit_impl.hh
edit src/cpu/o3/lsq_unit_impl.hh
set splitbelow splitright
wincmd t
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
argglobal
if bufexists("src/cpu/o3/lsq_unit_impl.hh") | buffer src/cpu/o3/lsq_unit_impl.hh | else | edit src/cpu/o3/lsq_unit_impl.hh | endif
if &buftype ==# 'terminal'
  silent file src/cpu/o3/lsq_unit_impl.hh
endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 1 - ((0 * winheight(0) + 15) / 31)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
1
normal! 0
lcd ~/projects/silent_stores_poc
tabnext 1
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0&& getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 winminheight=1 winminwidth=1 shortmess=filnxtToOFAI
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
nohlsearch
let g:this_session = v:this_session
let g:this_obsession = v:this_session
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
