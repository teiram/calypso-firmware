# Calypso Firmware

This firmware supports the middleboard [Calypso](https://github.com/teiram/calypso-cyc1000-board) featuring Mist compatibility.

In order to encourage experimentation and alternate approaches, the firmware is organized in the following way:
- Folder src/core. Provides the C++ foundations of the firmware. A sort of hardware abstraction layer that can be used to easily manage the rp2040 peripherals.
- Folder src/mist. Provides the Mist adaptation layer so that the oficial mist-firmware can find all the required dependencies and run. The mist firmware lifecycle is implemented in the class `calypso::MistService`
- Folder libs. 3rd party libraries or modules are expected to be added here (so far only mist-firmware is provided)
- src/main.cxx. This is the core of the firmware, where the different componentes are instantiated. Elements with lifecycle can be implemented as `calypso::Service` subclasses (like the existing MistService or USBService), registered in the Service repository and main will take care of their initialization and updates.

The official Mist firmware is included as a submodule, it's the official unmodified version that can be found in https://github.com/mist-devel/mist-firmware


