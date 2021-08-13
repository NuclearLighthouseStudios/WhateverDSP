#ifndef TWAIT_H
#define TWAIT_H

#include "debug.h"

#define TIMEOUT_WAIT(_cond, _timeout) ({\
long int count = _timeout;\
while(_cond){if(--count <= 0){panic("Timout waiting for %s\n", #_cond);}}})

#endif