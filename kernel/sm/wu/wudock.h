#if (_MSC_VER > 1000)
#pragma once
#endif

#ifndef WUDOCK_INCLUDED
#define WUDOCK_INCLUDED

#include "wutypes.h"
#include "wuerr.h"
#include "common/xptr.h" 
#include "common/errdbg/exceptions.h"

XPTR WuInternaliseXptr(const xptr& v);
xptr WuExternaliseXptr(XPTR v);
void WuSetLastExceptionObject(const SednaException &e);
void WuThrowException();

#define WU_CATCH_EXCEPTIONS() \
	catch (SednaException &e) \
	{ \
		WuSetLastExceptionObject(e); \
	} \
	catch (...) \
	{ \
		WuSetLastErrorMacro(WUERR_UNKNOWN_EXCEPTION); \
	}

#endif