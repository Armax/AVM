# AVM
This is a VM with a small 16 bit instruction set, an assembler and an assembly language that I wrote for fun and excercise (still a lot to do)<br>
(I will write a full documented readme in the next days)<br>

## A4X
This is the assembly language used with avm_assembler, an example of a factorial program can be found in src/factorial.a4x
<br>
## Compilation
I only use stdio string and stdlib so you just need to
```
gcc main.c -o avm && gcc assembler/main.c -o avm_assembler
```
in order to compile both VM and Assembler
<br>
## Running a binary in the AVM
First of all you should write a program with the A4X syntax, at the moment check main.c in the root directory for a4x documentation or look at the example in src/factorial.a4x, then use the avm_assembler to compile it
```
./avm_assembler src/factorial.a4x factorial
```
Now you can start the AVM:
```
./avm
```
now just paste the binary path and press enter to start the execution
<br>
### Contact me
[@Arm4x](https://twitter.com/Arm4x)
Feel free to contact me for help or anything else
