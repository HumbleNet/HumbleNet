/*   
Copyright 2006 - 2015 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#if !defined(_NOHECI)

#ifndef __ILibLMS__
#define __ILibLMS__
#include "ILibAsyncServerSocket.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
void __fastcall ILibLMS_setregistryA(char* name, char* value);
int __fastcall ILibLMS_getregistryA(char* name, char** value);
int __fastcall ILibLMS_deleteregistryA(char* name);
#endif

struct ILibLMS_StateModule *ILibLMS_Create(void *Chain, char* SelfExe);
int info_GetMeInformation(char** data, int loginmode);
int info_GetAmtVersion();

#ifdef __cplusplus
}
#endif

#endif

#endif

