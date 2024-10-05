# Calypso Firmware

This firmware supports the middleboard [Calypso](https://github.com/teiram/calypso-cyc1000-board) featuring Mist compatibility.

In order to encourage experimentation and alternate approaches, the firmware is organized in the following way:
- Folder src/core. Provides the C++ foundations of the firmware. A sort of hardware abstraction layer that can be used to easily manage the rp2040 peripherals.
- Folder src/mist. Provides the Mist adaptation layer so that the oficial mist-firmware can find all the required dependencies and run. The mist firmware lifecycle is implemented in the class `calypso::MistService`
- Folder libs. 3rd party libraries or modules are expected to be added here (so far only mist-firmware is provided)
- src/main.cxx. This is the core of the firmware, where the different componentes are instantiated. Elements with lifecycle can be implemented as `calypso::Service` subclasses (like the existing MistService or USBService), registered in the Service repository and main will take care of their initialization and updates.

The official Mist firmware is included as a submodule, it's the official unmodified version that can be found in https://github.com/mist-devel/mist-firmware

# Compiling from sources
## Requirements
- Pico SDK 2 properly installed and configured. If you are able to build the pico examples you are good to go. Please refer to [this page](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
- It should work regardless of your operating system. I'm using Linux Mint 21.3
## Procedure
- Clone this repository
```
  git clone https://github.com/teiram/calypso-firmware
```
- Create an empty build directory (you can do it inside the repository itself since `build` is in .gitignore blacklist but another directory outside of the repository folder is also okey
- Create the makefiles with the cmake utility, providing as argument the source location (the location of the top CMakeLists.txt file). For instance if you created the build directory inside the repository itself, it would be like:
```
    cd build
    cmake ..
```   
- Compile the firmware. Just run make inside the build directory
- Install the firmware on your rp2040. If you have another pico as SWD host connected to the pico on the calypso board, and a working openocd tool, you can just run:
```
    make upload
```
  otherwise you can also upload the firmware via USB (press boot and reset on the USB connected RP2040 and drop the file on the opening Mass Storage device) using the generated file
```
    build/src/calypso-firmware.uf2
```
