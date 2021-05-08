# Miabot Tracker Player Driver

This is a nostalgic commit of the Miabot player tracker driver I wrote from my PhD.
Worked with Player 2.1 (http://playerstage.sourceforge.net/)

## Build

To compile the tracker plugin type the following commands at the command prompt.

```
$ g++ `pkg-config --cflags playercore` -o tracker.so -shared tracker.cc `pkg-config --libs playercore`
```

## Run

```
$ player tracker.cfg
```
