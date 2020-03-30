#ifndef  __EMS_COMMON_HEADER__
#define  __EMS_COMMON_HEADER__

#ifdef __USETR1LIB__
#include <tr1/unordered_map>
#define unordered_map std::tr1::unordered_map
#else
#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map
#endif

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <string.h>
#include <vector>
#include <time.h>

#include "EmsDefine.h"

#endif  //__EMS_COMMON_HEADER__
