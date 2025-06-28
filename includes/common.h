#pragma once
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <vector>
#include <map>
#include <ctype.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#pragma warning(disable:4996)
#pragma warning(disable:4244)
#endif

#endif // !COMMON_H

