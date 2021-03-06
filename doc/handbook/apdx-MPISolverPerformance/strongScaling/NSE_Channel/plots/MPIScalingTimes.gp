set output 'MPIScalingTimes.tex'
set terminal cairolatex  pdf   size 17cm,17cm
set multiplot
set size 1,0.5
set size 0.98,0.49
set origin 0.01,0.505
unset lmargin 
unset rmargin 
unset tmargin 
set bmargin 1e00
unset logscale x
unset logscale y
unset logscale x2
unset logscale y2
set xrange [1:64]
set autoscale y
set autoscale x2
set autoscale y2
unset xlabel
set ylabel "Time [s]"
unset x2label
set y2label "SlvIter"
set title "Exclusive times"
unset key
set key outside right vertical maxrows 3 
set xtics 
set x2tics format " " 
set ytics 
set y2tics format " " 
set termoption dashed
set termoption dashed
set termoption dashed
plot "MPIScalingTimes_data_0.csv" title "NewtonGmres Automatic MGLevels3" with linespoints linecolor  "black" dashtype 3 linewidth 3 pointtype 6 pointsize 0.5, "MPIScalingTimes_data_1.csv" title "NewtonGmres Swz w Coarse Overlap MGLevels3" with linespoints linecolor  "black" dashtype 3 linewidth 3 pointtype 4 pointsize 0.5, "MPIScalingTimes_data_2.csv" title "NewtonGmres Swz w Coarse MGLevels3" with linespoints linecolor  "black" dashtype 4 linewidth 3 pointtype 11 pointsize 0.5
set size 0.98,0.49
set origin 0.01,0.005
unset lmargin 
unset rmargin 
set tmargin 1e00
unset bmargin 
unset logscale x
unset logscale y
unset logscale x2
unset logscale y2
set xrange [1:64]
set autoscale y
set autoscale x2
set autoscale y2
set xlabel "Processors"
set ylabel "Time [s]"
unset x2label
set y2label "SlvInit"
unset title 
unset key
set key outside right vertical maxrows 3 
set xtics 
set x2tics format " " 
set ytics 
set y2tics format " " 
set termoption dashed
set termoption dashed
set termoption dashed
plot "MPIScalingTimes_data_3.csv" title "NewtonGmres Automatic MGLevels3" with linespoints linecolor  "black" dashtype 3 linewidth 3 pointtype 6 pointsize 0.5, "MPIScalingTimes_data_4.csv" title "NewtonGmres Swz w Coarse Overlap MGLevels3" with linespoints linecolor  "black" dashtype 3 linewidth 3 pointtype 4 pointsize 0.5, "MPIScalingTimes_data_5.csv" title "NewtonGmres Swz w Coarse MGLevels3" with linespoints linecolor  "black" dashtype 4 linewidth 3 pointtype 11 pointsize 0.5


exit
