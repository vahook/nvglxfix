# nvglxfix

## What's this

A possible fix for the random nvidia linux driver hangs on X. Might also be related to https://github.com/ValveSoftware/gamescope/issues/1592 (NV 4924590) .

It fixes a race condition in nvidia's GLX implementation (`libGLX_nvidia.so`) that may cause the application's rendering to freeze. It may not effect all hardware / software combinations.

## Using it

Just add the library to `LD_PRELOAD`, something like this:
```
LD_PRELOAD="/path/to/libnvglxfix.so ${LD_PRELOAD}" my_application
```
Note that it should be first in the preload chain, as otherwise something else might resolve the real `dlsym` before we load, making our attempt at hooking it fail. (This is usually the case with wine / proton).

## Reproducing the bug

The race involves an XSyncFence reset request from the X client and a "damage trigger" event coming from the nvidia driver. You can help the "damage trigger" event win the race by spamming the X server with requests, causing the client's rendering to lock up.

To reliably reproduce the bug (given that your hardware / software combination is affected):
1. launch the built `xspam` executable (`./xspam`) to start spamming the X server with requests.
2. launch an application on the nvidia GPU that's prone to locking up (for example a game using DXVK) **without using the fix**.
3. observe the application almost instantly freezing.

You may now also try step 2., but try using the fix.

## Building

```sh
make
```

or

```sh
mkdir build && cd build
cmake .. && make
```
