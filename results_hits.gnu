set terminal pngcairo enhanced font 'arial,10' fontscale 1.5 size 1024, 768
set output '/home/flav/OrCS/results_hits.png'
unset border
set grid
set style fill  solid 0.25 noborder
set boxwidth 0.75 absolute
set title 'Hits'
set xlabel  'Benchmark'
set ylabel  '% Hits'
set style histogram clustered 
set style data histograms
set key under autotitle nobox
set yrange [0:]
set xtics  norangelimit
set xtics   ()
plot  '/home/flav/OrCS/1_bit_hits.dat' u 3:xtic(1) with histogram title '1-Bit', \
'/home/flav/OrCS/2_bit_hits.dat' u 3:xtic(1) with histogram title '2-bit', \
'/home/flav/OrCS/perceptron_hits.dat' u 3:xtic(1) with histogram title 'Perceptron', \
'/home/flav/OrCS/pb_perceptron_hits.dat' u 3:xtic(1) with histogram title 'Path-Based Perceptron', \
'/home/flav/OrCS/btb_hits.dat' u 3:xtic(1) with histogram title 'BTB'