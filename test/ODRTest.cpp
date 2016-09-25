/*
* Vool - include all modules
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

// including ODRTest.h will load all module headers again,
// which should trigger ODR violation
#include "ODRTest.h"