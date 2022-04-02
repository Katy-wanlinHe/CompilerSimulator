# CompilerSimulator
Simple simulator that processes LC4 commands

The project contains 2 parts. Part 1 (trace.c) is about building an OBJ file parser and loading its contents into an array 
representing the LC4’s memory. Part 2 (LC4.c) uses the contents of the LC4’s memory (the populated memory array from part 1) 
and executes the program that was stored in the OBJ file. 
