# Profiling using Perf

`perf stat ./server` to start running the program, then in another terminal send a command using `./client 127.0.0.1 1234 set key1 val1`.

This should display something along the lines of:


```
 Performance counter stats for './server':

              4.02 msec task-clock:u                     #    0.002 CPUs utilized
                 0      context-switches:u               #    0.000 /sec
                 0      cpu-migrations:u                 #    0.000 /sec
               136      page-faults:u                    #   33.840 K/sec
           1990172      cycles:u                         #    0.495 GHz
             23822      stalled-cycles-frontend:u        #    1.20% frontend cycles idle      
             38578      stalled-cycles-backend:u         #    1.94% backend cycles idle       
           2266097      instructions:u                   #    1.14  insn per cycle
                                                  #    0.02  stalled cycles per insn   
            355192      branches:u                       #   88.380 M/sec
             11283      branch-misses:u                  #    3.18% of all branches

       2.578321626 seconds time elapsed

       0.003813000 seconds user
       0.000000000 seconds sys

```


`perf record ./server` to generate a `perf.data` file which can be analyzed with `perf report`
```
# Total Lost Samples: 0
#
# Samples: 16  of event 'cycles:Pu'
# Event count (approx.): 1717530
#
# Overhead  Command  Shared Object         Symbol
# ........  .......  ....................  ..............................................
#
    35.34%  server   ld-linux-x86-64.so.2  [.] _dl_relocate_object
    28.68%  server   libstdc++.so.6.0.30   [.] std::ctype<wchar_t>::_M_initialize_ctype()
    20.28%  server   ld-linux-x86-64.so.2  [.] check_match
    12.02%  server   ld-linux-x86-64.so.2  [.] do_lookup_x
     2.93%  server   [unknown]             [k] 0xffffffffba2c2067
     0.60%  server   ld-linux-x86-64.so.2  [.] brk
     0.12%  server   ld-linux-x86-64.so.2  [.] _dl_start
     0.02%  server   [unknown]             [k] 0xffffffffbb000be0
     0.00%  server   [unknown]             [k] 0xffffffffbb000be3
     0.00%  server   [unknown]             [k] 0xffffffffba2c7095
     0.00%  server   [unknown]             [k] 0xffffffffba160002
     0.00%  server   [unknown]             [k] 0xffffffffba2c70e3

```


The above shows that over 70% of time is spent in dynamic linking operations (ld-linux-x86-64.so.2).
I have a feeling this is because of the heavy use of templates for our CRTP server. Let's check with our old server.

```
Performance counter stats for './server':

              4.30 msec task-clock:u                     #    0.001 CPUs utilized
                 0      context-switches:u               #    0.000 /sec
                 0      cpu-migrations:u                 #    0.000 /sec
               134      page-faults:u                    #   31.166 K/sec
           2108965      cycles:u                         #    0.491 GHz
             40009      stalled-cycles-frontend:u        #    1.90% frontend cycles idle      
             13929      stalled-cycles-backend:u         #    0.66% backend cycles idle       
           2258792      instructions:u                   #    1.07  insn per cycle
                                                         #    0.02  stalled cycles per insn
            352836      branches:u                       #   82.063 M/sec
             11871      branch-misses:u                  #    3.36% of all branches

       3.178487382 seconds time elapsed

       0.004282000 seconds user
       0.000000000 seconds sys
```


```
# Total Lost Samples: 0
#
# Samples: 26  of event 'cycles:Pu'
# Event count (approx.): 1772240
#
# Overhead  Command  Shared Object         Symbol                                                                                                               >
# ........  .......  ....................  .....................................................................................................................>
#
    27.84%  server   ld-linux-x86-64.so.2  [.] _dl_fixup
    22.21%  server   ld-linux-x86-64.so.2  [.] do_lookup_x
    20.05%  server   ld-linux-x86-64.so.2  [.] _dl_lookup_symbol_x
    10.11%  server   libm.so.6             [.] log2f@@GLIBC_2.27
     9.81%  server   ld-linux-x86-64.so.2  [.] strcmp
     4.26%  server   server                [.] std::vector<unsigned char, std::allocator<unsigned char> >::size() const
     3.70%  server   [unknown]             [k] 0xffffffffbb000be0
     0.85%  server   ld-linux-x86-64.so.2  [.] update_active.constprop.0
     0.74%  server   libstdc++.so.6.0.30   [.] malloc@plt
     0.21%  server   libc.so.6             [.] __fcntl64_nocancel_adjusted
     0.15%  server   ld-linux-x86-64.so.2  [.] __GI___tunables_init
     0.04%  server   server                [.] ISocketWrapper* const& std::__get_helper<0ul, ISocketWrapper*, std::default_delete<ISocketWrapper> >(std::_Tuple_>
     0.03%  server   [unknown]             [k] 0xffffffffba8a4fa0
     0.01%  server   server                [.] Server::handle_new_connections()
     0.01%  server   ld-linux-x86-64.so.2  [.] __rtld_malloc_init_stubs
     0.00%  server   [unknown]             [k] 0xffffffffba2c7095
     0.00%  server   server                [.] Server::start()
     0.00%  server   server                [.] std::_Head_base<0ul, IEpollWrapper*, false>::_M_head(std::_Head_base<0ul, IEpollWrapper*, false> const&)
     0.00%  server   server                [.] EpollWrapper::wait()
     0.00%  server   server                [.] IEpollWrapper* const& std::__get_helper<0ul, IEpollWrapper*, std::default_delete<IEpollWrapper> >(std::_Tuple_imp>
     0.00%  server   server                [.] std::unique_ptr<IEpollWrapper, std::default_delete<IEpollWrapper> >::operator->() const
     0.00%  server   [unknown]             [k] 0xffffffffba160019
     0.00%  server   [unknown]             [k] 0xffffffffba2c70a9
     0.00%  server   [unknown]             [k] 0xffffffffbb001256

```
