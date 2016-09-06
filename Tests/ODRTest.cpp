/*
* Vool - include all modules
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

// including ODRTest.h will load all module headers again,
// which should trigger ODR violation
#include "ODRTest.h"