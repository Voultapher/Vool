/*
* Vool - include all modules
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
*/

// including ODRTest.h will load all module headers again,
// which should trigger ODR violation
#include "ODRTest.h"