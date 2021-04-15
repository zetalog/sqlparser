// $Header: /home/cvsroot/eps-neo/include/argcv.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
//
// Copyright(C) 2004 NECAS Shanghai Development Center, NEC System Technologies and NEC Corporation.
//

// argcv.h: command line parsing routines

// This file was created by NECAS for SecureVisor Linux appliance initialization.
//
// 04/10/22 1 Refined by HUANG Ying <huangy@sh.necas.nec.com.cn>
//

#ifndef _ARGCV_H_
#define _ARGCV_H_

#ifdef __cplusplus
extern "C" {
#endif

char **argcv_new(int argc);
int argcv_free(int argc, char **argv);
void argcv_set(int argc, char **argv, int index, const char *value);
int argcv_get(const char *command, const char *delim, int *argc, char ***argv);
int argcv_put(char *command, int size, char *delim, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
