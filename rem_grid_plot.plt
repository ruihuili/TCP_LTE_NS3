set view map;
set terminal eps
set output "rem_grid.eps"
set xlabel "X [m]"
set ylabel "Y [m]"
set cblabel "SINR (dB)"
unset key
plot 'lte-grid.rem' using ($1):($2):(10*log10($4)) with image
