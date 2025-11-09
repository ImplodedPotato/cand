# A Simple Sand And Liquid Sim Game Made With [Raylib](https://www.raylib.com/)

## To Build:

### For PLATFORM_DESKTOP

``` Bash
./run.sh
```

### For PLATFORM_WEB

``` Bash
./run.sh web
```
#### WARNING

To build a platform right after another, you must pass in the ```-B``` flag to fully rebuild everything for
the given plaform, for example: ```./run.sh web -B```.

#### Other Notes

- The ```run.sh``` script just runs make and will work on MacOS & Linux, given that you have make.
- The Makefile is only tested on Macos and Wasm, may work on Linux, but probably wont work on Windows.
- The whole game is contained within a single file ```main.c```, along with a helper ```types.h```, and has
not been organized.
