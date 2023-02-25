# CDC 1604 Simulator 💾

A simulator to emulate the CDC 1604 computer. 

![cdc_1604 c1959 102635877 lg](https://user-images.githubusercontent.com/58179546/220802944-0df441b0-dbed-4baf-ae75-776cef5f38b2.jpg)

Details on the CDC 1604 can be found either on [Wikipedia](https://en.wikipedia.org/wiki/CDC_1604) or from the [CDC 1604 Manual](http://bitsavers.org/pdf/cdc/1604/018c_CDC1604_Manual.pdf)

## ❗ Important Notes ❗

- You can not compile with any sanitizer flags but "gcc main.c -std=gnu18 -Wall" will work on Windows/Ubuntu
- There are a lot of warnings when compiled with -Wall, this is due to me printing out variables as %llo (long long octals) when they come from a different format. There was issues when trying to type cast so I had to deal with the warnings
- The first line of the given octal file must be either 16 digits or must contain a comment afterwards, a bug I did not realise I had until last minute and it's too late to debug...
- Non-blocking I/O is not included but I have an alternative for it listed in the description.
- Only has the instructions required for the basic simulator

## Description 📝

This is my basic simulator of the CDC 1604 computer. It will run given octal files that use only instructions from the basic simulator portion of the project. After giving all the file names you wish to load, specify the PC to start your program at and then you will be granted a list of simulator options (memory dump, set memory location, see memory locations, set PC, etc). After putting the start lever down you will be asked to run in single step mode.

If you choose to run in single step mode, it will fetch and execute one instruction and then will prompt you to continue by entering 'c' or you can lift the lever by enter 'G' which will allow you to use simulator options until you lower the start lever again with 'g'

If you choose not to run in single step, the simulator will bein executing instructions as quickly as it can and you will be unable to perform any simulator options until either a breakpoint is hit, in which case you may lift the start lever with 'G' to regain control or continue running by pressing 'c'

In either mode after each instruction executes you will see the contents of the A and Q registers, the index registers, the current memory location being executed, and the instruction (opcode, b bits, and m bits).

## Any Problems? 🙋‍♂️

If there are any problems operating the simulator please consider emailing me (aiden.timmons@unb.ca) to discuss the issue that perhaps works on my machine but I did not forsee the issue on another machine. Thank you.
