/* This file is automatically generated.  DO NOT EDIT! */
/* Generated from: NetBSD: mknative-gcc,v 1.114 2021/04/11 07:35:45 mrg Exp  */
/* Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp  */

#include "configargs.h"

#define GCCPLUGIN_VERSION_MAJOR   10
#define GCCPLUGIN_VERSION_MINOR   3
#define GCCPLUGIN_VERSION_PATCHLEVEL   0
#define GCCPLUGIN_VERSION  (GCCPLUGIN_VERSION_MAJOR*1000 + GCCPLUGIN_VERSION_MINOR)

static char basever[] = "10.3.0";
static char datestamp[] = "20210408";
static char devphase[] = "";
static char revision[] = "";

/* FIXME plugins: We should make the version information more precise.
   One way to do is to add a checksum. */

static struct plugin_gcc_version gcc_version = {basever, datestamp,
						devphase, revision,
						configuration_arguments};
