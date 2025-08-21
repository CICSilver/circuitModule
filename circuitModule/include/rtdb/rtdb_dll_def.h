#ifndef _RTDB_DLL_DEF_H_
#define _RTDB_DLL_DEF_H_

#ifdef WIN32
	#define ExpFunc	__declspec( dllexport )
	#define ExpClass class __declspec( dllexport )
#else
	#define ExpFunc	extern
	#define ExpClass class
#endif


#endif