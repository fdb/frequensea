## Frequensea

    Visualize the frequency spectrum.

## Installing dependencies (OS X)

    brew update
    brew install cmake lua homebrew/versions/glfw3 fftw librtlsdr
    brew tap robotastic/homebrew-hackrf
    brew install --HEAD hackrf

## Building

    mkdir build
    cd build/
    cmake ..
    make

## Running

    ./frequensea ../lua/static.lua

With the Oculus:

    ./frequensea --vr ../lua/static.lua

## Build and Run

    make && ./frequensea ../lua/static.lua
