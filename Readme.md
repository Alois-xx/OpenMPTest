## Test Application for OpenMP and other things

## Event Test
When two threads are signalling at a rapid rate the performance is dominated by the context switch performance. 
With the introduction of C-States things can be slowed down a lot. The test will do thread ping-pong.
```
 0.0 Start two threads
 1.1 Thread 1 Wait for signal
 2.1 Thread 1 Signal Thread 2
 3.1 Thread 1 Goto 1.1
 1.2 Thread 2 Wait for signal
 2.2 Thread 2 Signal Thread 1
 3.2 Thread 2 Goto 1.2
```
The tests were executed on Windows 11 (10.0.22621.2715) Intel I5 13600K.

### Balanced Power Plan
```
OpenMPTest.exe -event 
Event Test of 100000 signals on two threads. Elapsed time: 0.9 s
```
### High Performance Power Plan
```
OpenMPTest.exe -event 
Event Test of 100000 signals on two threads. Elapsed time: 0.8 s
```

### Power Saver Power Plan
```
OpenMPTest.exe -event 
Event Test of 100000 signals on two threads. Elapsed time: 1.9 s
```

When the time is < 3s then only C1/C1E power states are enabled. If the time goes up to ca. 16s then 
the OS transitions to deep sleep states which need ca. 140us per Context Switch to wake up the CPU. This can happen
even when the High Performance Power plan is enabled e.g. on Windows Server 2022. 

You can switch between power plans without a reboot to quickly test the effects of the power settings. 

## OpenMP Test
This will create many small sized OpenMP (Intel OpenMP library) tasks which need a lot of synchronization by the OS if no CPU spinning is configured
via the environment variable KMP_BLOCKTIME.
Depending on the number of threads and Power Profile (C-States)

The tests were executed on Windows 11 (10.0.22621.2715) Intel I5 13600K.

### Power Saver
```
OpenMPTest.exe 0 
Setting KMP_BLOCKTIME to 0
OpenMP loop with 500000 outer and 10000 parallel inner loop. Elapsed time: 46.3 s

OpenMPTest.exe 1 
Setting KMP_BLOCKTIME to 1
OpenMP loop with 500000 outer and 10000 parallel inner loop. Elapsed time: 4.5 s
```
### High Performance
```
OpenMPTest.exe 0 
Setting KMP_BLOCKTIME to 0
OpenMP loop with 500000 outer and 10000 parallel inner loop. Elapsed time: 18.5 s

OpenMPTest.exe 1 
Setting KMP_BLOCKTIME to 1
OpenMP loop with 500000 outer and 10000 parallel inner loop. Elapsed time: 1.3 s
```
### Balanced 

```
OpenMPTest.exe 0 
Setting KMP_BLOCKTIME to 0
OpenMP loop with 500000 outer and 10000 parallel inner loop. Elapsed time: 18.2 s

OpenMPTest.exe 1 
Setting KMP_BLOCKTIME to 1
OpenMP loop with 500000 outer and 10000 parallel inner loop. Elapsed time: 1.3 s
```

## Pause Latency 



### Sleeping is complicated
The CPU pause instruction is defined as a number of cycles which a processor is put to sleep. This value has changed
between CPU generations. 

Pause latency from https://uops.info/table.htm in cycles:

| Haswell  | Broadwell | Skylake-X | Kaby Lake | Coffee Lake | Cannon Lake | Cascade Lake | Ice Lake | Tiger Lake | Rocket Lake | Alder Lake-P | AMD Zen2 | AMD Zen4 |
| ---------|---------  |---------  |---------  |---------    |---------    |---------     |--------- |---------   |---------    |---------     |--------- |--------- |
| 9.00     |  9.00     |  140.00   | 140.00    |  152.50     |  157.00     |  40.00       |  138.20  | 138.20     | 138.20      |  160.17      | 65.00    | 65.00    |

Since the numbers are defined in cycles the actual duration depends on the current clock speed of the CPU.
Server CPU frequencies have been relatively constant but with the introduction of Priority Cores a similar concept of P/E cores
has arrived at servers which makes the previously constant latency of the pause instruction more dynamic.

### Intel I7 13600K Raptor Lake (5.1 GHz P-Core 3.9 GHz E-Core)
```
C:\Source\OpenMPTest_1.0.0.1\OpenMPTest.exe -pause 
Pause Latency CPU[0]: 33.5 ns
Pause Latency CPU[1]: 34.0 ns
Pause Latency CPU[2]: 32.7 ns
Pause Latency CPU[3]: 32.9 ns
Pause Latency CPU[4]: 33.2 ns
Pause Latency CPU[5]: 33.0 ns
Pause Latency CPU[6]: 32.8 ns
Pause Latency CPU[7]: 34.2 ns
Pause Latency CPU[8]: 32.9 ns
Pause Latency CPU[9]: 33.2 ns
Pause Latency CPU[10]: 32.9 ns
Pause Latency CPU[11]: 32.9 ns
Pause Latency CPU[12]: 15.8 ns
Pause Latency CPU[13]: 15.7 ns
Pause Latency CPU[14]: 15.7 ns
Pause Latency CPU[15]: 15.8 ns
Pause Latency CPU[16]: 15.7 ns
Pause Latency CPU[17]: 16.0 ns
Pause Latency CPU[18]: 15.9 ns
Pause Latency CPU[19]: 16.0 ns
```

This gets even more complex because pause latency not only depends on CPU generation and frequency but also if code is running on a Power (P) or Efficiency (E) core.

### Intel I7 13700 Raptor Lake
```
C:\source>"C:\source\vc22\OpenMPTest_1.0.0.1\OpenMPTest.exe" -pause 
Pause Latency CPU[0]: 54.6 ns
Pause Latency CPU[1]: 45.9 ns
Pause Latency CPU[2]: 54.5 ns
Pause Latency CPU[3]: 54.9 ns
Pause Latency CPU[4]: 52.9 ns
Pause Latency CPU[5]: 54.8 ns
Pause Latency CPU[6]: 54.7 ns
Pause Latency CPU[7]: 54.8 ns
Pause Latency CPU[8]: 54.8 ns
Pause Latency CPU[9]: 55.0 ns
Pause Latency CPU[10]: 54.6 ns
Pause Latency CPU[11]: 55.1 ns
Pause Latency CPU[12]: 54.5 ns
Pause Latency CPU[13]: 54.8 ns
Pause Latency CPU[14]: 54.6 ns
Pause Latency CPU[15]: 54.8 ns
Pause Latency CPU[16]: 14.4 ns
Pause Latency CPU[17]: 14.5 ns
Pause Latency CPU[18]: 14.5 ns
Pause Latency CPU[19]: 14.5 ns
Pause Latency CPU[20]: 14.5 ns
Pause Latency CPU[21]: 14.5 ns
Pause Latency CPU[22]: 14.6 ns
Pause Latency CPU[23]: 14.5 ns
```

### Xeon Gold 5415+ 
```
C:\temp\OpenMPTest_1.0.0.1>OpenMPTest.exe -pause
Pause Latency CPU[0]: 10.0 ns
Pause Latency CPU[1]: 10.5 ns
Pause Latency CPU[2]: 10.5 ns
Pause Latency CPU[3]: 10.3 ns
Pause Latency CPU[4]: 10.3 ns
Pause Latency CPU[5]: 10.3 ns
Pause Latency CPU[6]: 10.3 ns
Pause Latency CPU[7]: 10.3 ns
Pause Latency CPU[8]: 10.3 ns
Pause Latency CPU[9]: 10.3 ns
Pause Latency CPU[10]: 10.3 ns
Pause Latency CPU[11]: 10.3 ns
Pause Latency CPU[12]: 10.0 ns
Pause Latency CPU[13]: 10.0 ns
Pause Latency CPU[14]: 10.2 ns
Pause Latency CPU[15]: 10.6 ns
Pause Latency CPU[16]: 10.1 ns
Pause Latency CPU[17]: 10.3 ns
Pause Latency CPU[18]: 10.0 ns
Pause Latency CPU[19]: 10.5 ns
Pause Latency CPU[20]: 10.2 ns
Pause Latency CPU[21]: 10.3 ns
Pause Latency CPU[22]: 10.3 ns
Pause Latency CPU[23]: 10.4 ns
Pause Latency CPU[24]: 10.0 ns
Pause Latency CPU[25]: 10.3 ns
Pause Latency CPU[26]: 10.2 ns
Pause Latency CPU[27]: 10.3 ns
Pause Latency CPU[28]: 10.0 ns
Pause Latency CPU[29]: 10.3 ns
Pause Latency CPU[30]: 10.0 ns
Pause Latency CPU[31]: 10.3 ns
```
