ESE 333 Real Time Operating Systems
Project 1

Author: Aditya Taday (109550833)

Software Information:

This is a C Program for interpreting basic unix commands specifically IO Redirection, Pipes and Background Processing
  
Recognizes single level of redirection and process operators '<', '>', '|', '&' 
  
Installation:

compile using : gcc -o shell ESE333_project1_Aditya_Taday.c 
Running the software: ./shell

Usage sample:

Input redirection: cat < file
Output redirection: echo hello > file
Pipe: man date | grep os
Backround: xterm &

Works with other unix shell commands such as cd, pwd, ls, mkdir, rmdir, etc...

Contact Information:

Email: aditya.taday@stonybrook.edu
