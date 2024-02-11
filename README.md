# ryo

This small utility is written for [kokanyctl](https://github.com/zsoltiv/kokanyctl),
and is used to remux an FFmpeg input to multiple UNIX domain sockets.
The program also recreates closed sockets ASAP.

## Requirements

- POSIX compliant system (for `getopt`)
- C99 compliant compiler
- make
- FFmpeg libraries (libav\*.so)
