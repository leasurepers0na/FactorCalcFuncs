#pragma once
#ifdef DLL_PROJECT_H
#pragma warning(disable:4251)
#ifdef DLLEXAM_EXPORTS
#define DLLEXAM_API __declspec(dllexport)
#else
#define DLLEXAM_API __declspec(dllimport)
#endif
#else
#define DLLEXAM_API
#endif
