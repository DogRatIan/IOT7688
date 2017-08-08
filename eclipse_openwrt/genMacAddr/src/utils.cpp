//==========================================================================
// Utilities
//==========================================================================
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

//==========================================================================
// Defines
//==========================================================================
#define PATH_PID_FILE                   "/tmp"

//==========================================================================
// Variables
//==========================================================================
// PID file
static char gPidFilename[1024] = "";

//==========================================================================
// Get 1ms tick counter
//==========================================================================
unsigned long GetTick (void)
{
    unsigned long ret;
    struct timeval curr_time;

    gettimeofday (&curr_time, NULL);

    ret = curr_time.tv_usec / 1000;
    ret += (curr_time.tv_sec * 1000);
    
    return ret;
}

//==========================================================================
// Calculate the number of tick elapsed
//==========================================================================
unsigned long TickElapsed (unsigned long aTick)
{
    unsigned long curr_tick;

    curr_tick = GetTick ();
    if (curr_tick >= aTick)
        return (curr_tick - aTick);
    else
        return (0xffffffff - aTick + curr_tick);
}

//==========================================================================
// Averaging int
//==========================================================================
double AveragingInt (int *aIntBuf, int *aNewValue, int aBufLen)
{
    double ret;
    int i = 0;

    ret = 0;
    for (i = 0; i < (aBufLen - 1); i ++)
    {
        if (aNewValue != NULL)
            aIntBuf[i] = aIntBuf[i + 1];
        ret += aIntBuf[i];
    }

    if (aNewValue != NULL)
        aIntBuf[i] = *aNewValue;
    ret += aIntBuf[i];
    
    ret /= aBufLen;

    return ret;
}


//==========================================================================
// Gen Date string
//==========================================================================
static const char *KStrMonth[12] = 
{
    "JAN",
    "FEB",
    "MAR",
    "APR",
    "MAY",
    "JUN",
    "JUL",
    "AUG",
    "SEP",
    "OCT",
    "NOV",
    "DEC"
};

void GenDateString (char *aDest, time_t aTimeStamp, int aDestLen, int aLong)
{
    struct tm tm;

    memcpy (&tm, localtime (&aTimeStamp), sizeof (struct tm));
    if (aLong)
        snprintf (aDest, aDestLen, "%04d/%s/%02d %02d:%02d",
                                tm.tm_year + 1900, KStrMonth[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min);
    else
        snprintf (aDest, aDestLen, "%s/%02d %02d:%02d", KStrMonth[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min);

}

//==========================================================================
// Gen Date string
//==========================================================================
void GenDateCode (char *aDest, time_t aTimeStamp, int aDestLen, int aWithTime)
{
    struct tm tm;

    memcpy (&tm, localtime (&aTimeStamp), sizeof (struct tm));
    if (aWithTime)
        snprintf (aDest, aDestLen, "%04d%02d%02d%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
    else
        snprintf (aDest, aDestLen, "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

//==========================================================================
// Get the Filename from a path
//==========================================================================
const char *FileNameOnly (const char *aSrc)
{
    int i = strlen (aSrc);
    while (i)
    {
        if ((aSrc[i] == '\\') || (aSrc[i] == '/'))
        {
            i++;
            break;
        }
        i--;
    }
    return &aSrc[i];
}

//==========================================================================
// Write PID file
//==========================================================================
int WritePidFile (const char *aName, const char *aPath)
{
    char buf[64];
    int pid_file;
    int ret;

    // Check name
    if (strlen (aName) == 0)
        return -1;

    // Form PID filename
    if (aPath == NULL)
        snprintf (gPidFilename, sizeof (gPidFilename), PATH_PID_FILE "/%s.pid", FileNameOnly (aName));
    else
        snprintf (gPidFilename, sizeof (gPidFilename), "%s/%s.pid", aPath, FileNameOnly (aName));

    // Check PID file exist
    if (access (gPidFilename, F_OK) == 0)
    {
        return -1;
    }

    // Write PID file
    pid_file = open (gPidFilename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (pid_file < 0)
    {
        return -1;
    }
    
    snprintf (buf, sizeof (buf), "%d\n", getpid ());
    if (write (pid_file, buf, strlen (buf)) < 0)
        ret = -1;
    else
        ret = 0;

    close (pid_file);

    return ret;
}

//==========================================================================
// Remove PID file
//==========================================================================
void RemovePidFile (void)
{
    if (strlen (gPidFilename) == 0)
        return;

    if (access (gPidFilename, F_OK) == 0)
        remove (gPidFilename);
}

//==========================================================================
// Check Cross 5 min
//==========================================================================
bool Cross5Min (time_t aCurrTime, time_t aLastTime)
{
    int interval_5min    = (5 * 60);

    if ((aCurrTime / interval_5min) != (aLastTime / interval_5min))
        return true;
    else
        return false;
}

//==========================================================================
// Check Cross 1 hour
//==========================================================================
bool Cross1Hour (time_t aCurrTime, time_t aLastTime)
{
    int interval_1hour   = (60 * 60);

    if ((aCurrTime / interval_1hour) != (aLastTime / interval_1hour))
        return true;
    else
        return false;
}

//==========================================================================
// Convert a HEX string to value
//==========================================================================
unsigned long FromHexString (char *aDest, int aDigit)
{
    unsigned long ret;
    unsigned char ch;

    ret = 0;
    while (aDigit)
    {
        if ((*aDest >= '0') && (*aDest <= '9'))
            ch = *aDest - '0';
        else if ((*aDest >= 'A') && (*aDest <= 'F'))
            ch = *aDest - 'A' + 10;
        else if ((*aDest >= 'a') && (*aDest <= 'f'))
            ch = *aDest - 'a' + 10;
        else
            break;
        ret <<= 4;
        ret |= (ch & 0xf);

        aDest ++;
        aDigit --;
    }

    return ret;
}

//==========================================================================
// Convert a value to BINARY string
//==========================================================================
void ToBinaryString (char *aDest, unsigned long aValue, int aDigit, bool aMsbFirst)
{
    unsigned long mask;

    if (aMsbFirst)
        mask = (0x01 << (aDigit - 1));
    else
        mask = 0x01;
    while (aDigit)
    {
        if (aValue & mask)
            *aDest = '1';
        else
            *aDest = '0';

        aDest ++;
        aDigit --;
        if (aMsbFirst)
        {
            mask >>= 1;
            if (aDigit)
            {
                if (mask & 0x88888888)
                {
                    *aDest = ' ';
                    aDest ++;
                }
            }
        }
        else
        {
            mask <<= 1;
            if (aDigit)
            {
                if (mask & 0x11111111)
                {
                    *aDest = ' ';
                    aDest ++;
                }
            }
        }
    }
    *aDest = 0;
}

//==========================================================================
// Convert string to lower case
//==========================================================================
void StringToLower (char *aSrc)
{
    while (*aSrc)
    {
        if ((*aSrc >= 'A') && (*aSrc <= 'Z'))
            *aSrc = *aSrc - 'A' + 'a';
        aSrc ++;
    }
}

//==========================================================================
// Convert string to upper case
//==========================================================================
void StringToUpper (char *aSrc)
{
    while (*aSrc)
    {
        if ((*aSrc >= 'a') && (*aSrc <= 'z'))
            *aSrc = *aSrc - 'a' + 'A';
        aSrc ++;
    }
}

//==========================================================================
// Trim tailing space of a string
//==========================================================================
void TrimTail (char *aStr)
{
    int i;

    i = strlen (aStr);
    while (i)
    {
        i --;
        if (aStr[i] != ' ')
            break;
        aStr[i] = 0;
    }
}
