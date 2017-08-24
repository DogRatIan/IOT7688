//==========================================================================
//==========================================================================
#ifndef _INC_UTILS_H
#define _INC_UTILS_H

//==========================================================================
//==========================================================================
#include <time.h>

//==========================================================================
//==========================================================================
unsigned long GetTick (void);
unsigned long TickElapsed (unsigned long aTick);
void GenDateString (char *aDest, time_t aTimeStamp, int aDestLen, int aLong = false);
void GenDateCode (char *aDest, time_t aTimeStamp, int aDestLen, int aWithTime = false);
double AveragingInt (int *aBuf, int *aNewValue, int aBufLen);
const char *FileNameOnly (const char *aSrc);
int WritePidFile (const char *aName, const char *aPath = NULL);
void RemovePidFile (void);
bool Cross5Min (time_t aCurrTime, time_t aLastTime);
bool Cross1Hour (time_t aCurrTime, time_t aLastTime);
unsigned long FromHexString (char *aDest, int aDigit);
void ToBinaryString (char *aDest, unsigned long aValue, int aDigit, bool aMsbFirst = true);
void StringToLower (char *aSrc);
void StringToUpper (char *aSrc);
void TrimTail (char *aStr);

//==========================================================================
//==========================================================================
#endif
