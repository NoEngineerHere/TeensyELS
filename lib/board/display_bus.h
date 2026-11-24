#pragma once

class TwoWire;
class SPIClass;

bool initializeDisplayBus();
TwoWire& displayI2cBus();
SPIClass& displaySpiBus();
