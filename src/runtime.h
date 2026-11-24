#pragma once

enum class RuntimeStartResult { Started, MotionFailed, InputFailed, DisplayFailed };
RuntimeStartResult startRuntime();
void serviceRuntimeForeground();

void motionTick();
void inputTick();
void displayTick();
bool motionServiceNeeded();
