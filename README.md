# baedit

Boot cmdline editor for Linux kernels with a compiled-in cmdline.  
Mainly inteded for use with [Wii Linux](https://github.com/Wii-Linux).  

A clone of [neagix](https://github.com/neagix)'s program of [the same name](https://github.com/neagix/wii-linux-ngx/tree/master/baedit).

However, neagix's was written in Go.  This version is written in C with the intent to be portable to more platforms.


# Current status

- Kernel reading and finding the marks works
- Printing the args works
- Replacing the args seems to simply not perform the write, for some reason.
