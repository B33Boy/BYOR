# Profiling using Perf

`perf stat ./server` to start running the program, then in another terminal send a command using `./client 127.0.0.1 1234 set key1 val1`.

This should display something along the lines of:


```
 Performance counter stats for './server':

              4.97 msec task-clock:u                     #    0.002 CPUs utilized
                 0      context-switches:u               #    0.000 /sec
                 0      cpu-migrations:u                 #    0.000 /sec
               140      page-faults:u                    #   28.170 K/sec
           2107942      cycles:u                         #    0.424 GHz
             24369      stalled-cycles-frontend:u        #    1.16% frontend cycles idle
             98717      stalled-cycles-backend:u         #    4.68% backend cycles idle
           2304251      instructions:u                   #    1.09  insn per cycle
                                                         #    0.04  stalled cycles per insn
            361621      branches:u                       #   72.762 M/sec
             12064      branch-misses:u                  #    3.34% of all branches

       3.253412314 seconds time elapsed

       0.000000000 seconds user
       0.004436000 seconds sys
```


`perf record ./server` to generate a `perf.data` file which can be analyzed with `perf report`
```
# Total Lost Samples: 0
#
# Samples: 17  of event 'cycles:Pu'
# Event count (approx.): 2254939
#
# Overhead  Command  Shared Object         Symbol
# ........  .......  ....................  ................................
#
    45.06%  server   ld-linux-x86-64.so.2  [.] do_lookup_x
    26.21%  server   libc.so.6             [.] __memset_avx2_unaligned_erms
    10.67%  server   ld-linux-x86-64.so.2  [.] _dl_relocate_object
    10.38%  server   ld-linux-x86-64.so.2  [.] check_match
     5.87%  server   ld-linux-x86-64.so.2  [.] strcmp
     1.45%  server   ld-linux-x86-64.so.2  [.] _dl_new_object
     0.30%  server   ld-linux-x86-64.so.2  [.] __GI___tunables_init
     0.05%  server   [unknown]             [k] 0xffffffffa2000be0
     0.01%  server   [unknown]             [k] 0xffffffffa12c32d5
     0.00%  server   ld-linux-x86-64.so.2  [.] _dl_start
     0.00%  server   [unknown]             [k] 0xffffffffa2104104
     0.00%  server   [unknown]             [k] 0xffffffffa10e598f
     0.00%  server   [unknown]             [k] 0xffffffffa12c3336
```
