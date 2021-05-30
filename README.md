# WhateverDSP

WhateverDSP framework

## DFU Flashing

On MacOS:

1. `brew install gcc-arm-embedded`
2. `brew install dfu-util`
3. Hook up the USB cable to the board, then while pressing "Boot" hook up the 9V power
4. In examples/filter, `make dfu`
