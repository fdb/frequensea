## Frequensea

Frequensea is an open-source toolkit for visualizing the electromagnetic spectrum.

![A sea of FFT data](https://raw.github.com/fdb/frequensea/master/screenshots/fft-sea.png)

## Features
- Fast core written in C, with Lua scripting on top.
- Support fort RTL-SDR and HackRF devices
- Support for Oculus Rift for viewing the spectrum in virtual reality.
- Support for OSC to communicate with other applications or devices.
- Basic building blocks for sampling, filtering and visualizing RF data.

## Installing dependencies (OS X)

    brew update
    brew install cmake lua glew homebrew/versions/glfw3 fftw librtlsdr libpng libsndfile
    brew tap fdb/homebrew-hackrf
    brew install hackrf

## Installing dependencies (Ubuntu 14.04 LTS)

    sudo apt-get install -y git cmake gcc g++ make libfftw3-dev libpng-dev libusb-1.0.0-dev pkg-config xorg-dev libglu1-mesa-dev libopenal-dev libglew-dev libhackrf-dev librtlsdr-dev

    # There is no GLFW3 package so install from source
    wget https://github.com/glfw/glfw/archive/3.1.zip
    unzip 3.1.zip
    cd glfw
    mkdir build
    cd build
    cmake ..
    make
    sudo make install
    sudo ldconfig

## Installing dependencies (Raspberry Pi)

    sudo apt-get install -y git cmake gcc g++ make libfftw3-dev libpng-dev libusb-1.0.0-dev pkg-config xorg-dev libglu1-mesa-dev libopenal-dev libglew-dev

    # There is no RTL-SDR package so install from source
    git clone git://git.osmocom.org/rtl-sdr.git
    cd rtl-sdr
    mkdir build
    cd build
    cmake ../
    make
    sudo make install
    sudo ldconfig

    # Disable default kernel driver
    sudo modprobe -r dvb_usb_rtl28xxu

Note that you might need to run as root to claim the graphics driver, especially if you don't use the default "pi" user.

## Building

    mkdir build
    cd build/
    cmake ..
    make

## Running

    ./frequensea ../lua/static.lua

With the Oculus:

    ./frequensea --vr ../lua/static.lua

Save the output to a PNG sequence:

    ./frequensea --capture ../lua/animate-camera.lua

## Build and Run

    make && ./frequensea ../lua/static.lua
