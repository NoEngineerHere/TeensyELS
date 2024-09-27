# Teensy Electronic leadscrew
This is an implementation of an electronic leadscrew using a Teensy microcrontroller.

# Architecture
We use [https://platformio.org/](platformio) as the infrastructure to build this project.

# Installation
You will currently need to setup platformio to build this, it can also handle installation to your teensy as well.
NOTE: you will have to modify some of the configuration to get this set up for your particular lathe, instructions coming soon.

# Testing
We aim to have the business logic of this testable locally without having to program any hardware or run tests on hardware (for the most part)
To test you just need to run the `pio test --environment native` command to run what tests we have written

## Debugging
To test this application, you will be limited to setting breakpoints on unit tests only as the Teensy series of boards do not support hardware debuggers directly.

The "blessed" way to debug is to:
- Use vscode
- Set your environment to "native" (this is the default)
- Ensure that 'gdb' is installed
- Set your breakpoints in your code to test
    - note: if you want to debug a single test, set the test name in `test_debug` in platformio.ini
- go to the debug panel in vscode and run the "PIO debug" target
- ???
- Profit!
