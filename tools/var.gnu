var_desc=system("echo $VAR_DESC")

print "enter"

set terminal png size 8000,4000
set output 'out.png'
set border linewidth 0.5
set grid
set ytics 128
set ytics add
set pointsize 1.5
set xlabel 'Cycles' 
set ylabel 'Addresses'

min_x = 18446744073709551616.
max_x = 0.
min_y = 18446744073709551616.
max_y = 0.
min(x,y) = (x < y) ? x : y
max(x,y) = (x > y) ? x : y

file_exists(file) = system("[ -f '".file."' ] && echo '1' || echo '0'") + 0

do for [t=0:3] {
  f = sprintf('acc-R-%s-thr-%d',var_desc,t)
  if (file_exists(f)) {
      stats f using 2:1
      min_x = min(min_x, STATS_min_x)
      max_x = max(max_x, STATS_max_x)
      min_y = min(min_y, STATS_min_y)
      max_y = max(max_y, STATS_max_y)
  }
}

do for [t=0:3] {
  f = sprintf('acc-W-%s-thr-%d',var_desc,t)
  if (file_exists(f)) {
      stats f using 2:1
      min_x = min(min_x, STATS_min_x)
      max_x = max(max_x, STATS_max_x)
      min_y = min(min_y, STATS_min_y)
      max_y = max(max_y, STATS_max_y)
  }
}

min_y_call = 18446744073709551616.

stats "calls" using 1:2
min_x = min(min_x, STATS_min_x)
max_x = max(max_x, STATS_max_x)
min_y_call = min(min_y_call, STATS_min_y)

set yr [min_y_call:max_y - min_y]
set xr [min_x:max_x]

set style line 1 lc rgb '#FF0000' pt 1   # +
set style line 2 lc rgb '#00FF00' pt 7   # circle

set style line 3 lc rgb '#0000FF'

set multiplot layout 1, 1

do for [t=0:1] {
  rr = sprintf('acc-R-%s-thr-%d',var_desc,t)
  ww = sprintf('acc-W-%s-thr-%d',var_desc,t)
  tr = sprintf('Read-thr%d',t)
  tw = sprintf('Write-thr%d',t)
  plot rr using 2:($1-min_y) title tr with points ls 1, \
       ww using 2:($1-min_y) title tw with points ls 2, \
       "calls" using 1:2:ytic(3) title "calls" with lines ls 3
}