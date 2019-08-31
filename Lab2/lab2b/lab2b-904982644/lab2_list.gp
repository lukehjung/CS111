#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "List-1: Throughput vs Number of Threads for Mutex and Spinlock"
set xlabel "Threads"
set logscale x 2
set xrange [0.5:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/$7) \
	title 'Mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'Spin-Lock' with linespoints lc rgb 'green'


set title "List-2: Average Time per operation vs Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'Average Time per Operation' with linespoints lc rgb 'green' , \
     "< grep 'list-none-m,[0-9]*,1000,1' lab2b_list.csv" using ($2):($8) \
	title 'Average Wait Time for Lock' with linespoints lc rgb 'red' , 
     
set title "List-3: Iterations that don't fail"
unset logscale x2
set xrange [0.5:]
set xlabel "Threads"
set ylabel "successful iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "red" title "unprotected", \
    "< grep 'list-id-m,[0-9]*,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "green" title "mutex", \
    "< grep 'list-id-s,[0-9]*,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "blue" title "spinlock", 

set title "List-4: Aggregated Throughput vs Number of Mutex Threads"
set xlabel "Threads"
set logscale x2
unset xrange
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_4.png'
plot \
   "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '1 list' with linespoints lc rgb 'blue', \
    "< grep 'list-none-m,[0-9]*,1000,4' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '4 lists' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,.[0-9]*,1000,8' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '8 lists' with linespoints lc rgb 'orange', \
    "< grep 'list-none-m,[0-9]*,1000,16' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '16 lists' with linespoints lc rgb 'green'


set title "List-5: Aggregated Throughput vs. The Number of Threads(Spin Lock)"
set logscale x 2
set xrange [0.5:] #xrange has to be greater than 0 for logscale
set xlabel "Threads"
set ylabel "Throughput(operations/sec)"
set logscale y 10
set output 'lab2b_5.png'
plot \
        "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '1 list' with linespoints lc rgb 'red', \
        "< grep 'list-none-s,[0-9]*,1000,4' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '4 lists' with linespoints lc rgb 'green', \
        "< grep 'list-none-s,.[0-9]*,1000,8' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '8 lists' with linespoints lc rgb 'blue', \
        "< grep 'list-none-s,[0-9]*,1000,16' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '16 lists' with linespoints lc rgb 'orange'

