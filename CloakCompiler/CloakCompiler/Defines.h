#pragma once
#ifndef CC_API_DEFINES_H
#define CC_API_DEFINES_H

#include "CloakEngine/Defines.h"

#ifdef CLOAKCOMPILER_EXPORTS

#define CLOAKCOMPILER_API_NAMESPACE

#ifdef _WIN32
#define CLOAKCOMPILER_API __declspec(dllexport)
#else
#define CLOAKCOMPILER_API
#endif

#else

#define CLOAKCOMPILER_API_NAMESPACE inline

#ifdef _WIN32
#define CLOAKCOMPILER_API __declspec(dllimport)
#else
#define CLOAKCOMPILER_API
#endif

#endif

#endif