/* Copyright (C) 1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#ifndef _PARAM_H
#define _PARAM_H

#include <features.h>
#include <limits.h>
#include <linux/limits.h>
#include <linux/param.h>

#include <sys/types.h>

#define MAXPATHLEN PATH_MAX

#ifndef NR_OPEN
#define NR_OPEN 32
#endif
#ifndef NR_FILE
#define NR_FILE 32
#endif

#endif
