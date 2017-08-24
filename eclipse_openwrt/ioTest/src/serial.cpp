//==========================================================================
// Serial Interface
//==========================================================================
//==========================================================================
// Naming conventions
// ~~~~~~~~~~~~~~~~~~
//                Class : Leading C
//               Struct : Leading T
//             Constant : Leading K
//      Global Variable : Leading g
//    Function argument : Leading a
//       Local Variable : All lower case
//==========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "serial.h"

//==========================================================================
// Defines
//==========================================================================
#define INVALID_HANDLE_VALUE    -1

//------------------------------
//------------------------------
struct TBaudTable               // Baud rate table
{
    const char *Str;            // String of config input by user
    unsigned long BaudValue;    // Value pass to Linux termios
    int BaudShow;               // Value return to user
};

struct TModeTable               // Comm mode table
{
    const char *Str;            // String of config input by user
    unsigned long ModeValue;    // Value pass to Linux termios
};

//==========================================================================
// Constants
//==========================================================================
static const struct TBaudTable KBaudTable[] = 
{
    {"1200", B1200, 1200},
    {"2400", B2400, 2400},
    {"4800", B4800, 4800},
    {"9600", B9600, 9600},
    {"14400", 14400, 14400},
    {"19200", B19200, 19200},
    {"28800", 28800, 28800},
    {"38400", B38400, 38400},
    {"57600", B57600, 57600},
    {"115200", B115200, 115200},
    {NULL, 0, 0}
};

static const struct TModeTable KModeTable[] = 
{
    {"N81", CS8},
    {"E81", CS8 | PARENB},
    {"O81", CS8 | PARENB | PARODD},
    {"N71", CS7},
    {"E71", CS7 | PARENB},
    {"O71", CS7 | PARENB | PARODD},
    {"N82", CS8 | CSTOPB},
    {"E82", CS8 | PARENB | CSTOPB},
    {"O82", CS8 | PARENB | PARODD | CSTOPB},
    {"N72", CS7 | CSTOPB},
    {"E72", CS7 | PARENB | CSTOPB},
    {"O72", CS7 | PARENB | PARODD | CSTOPB},
    {"8N1", CS8},
    {"8E1", CS8 | PARENB},
    {"8O1", CS8 | PARENB | PARODD},
    {"7N1", CS7},
    {"7E1", CS7 | PARENB},
    {"7O1", CS7 | PARENB | PARODD},
    {"8N2", CS8 | CSTOPB},
    {"8E2", CS8 | PARENB | CSTOPB},
    {"8O2", CS8 | PARENB | PARODD | CSTOPB},
    {"7N2", CS7 | CSTOPB},
    {"7E2", CS7 | PARENB | CSTOPB},
    {"7O2", CS7 | PARENB | PARODD | CSTOPB},
    {NULL, 0}
};

//==========================================================================
//==========================================================================
static struct termios *gOldTio = NULL;

//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================

//==========================================================================
// Create of Object
//==========================================================================
CSerial::CSerial (void) :
    baudRate (9600),
    mode ("N81"),
    fdComPort (INVALID_HANDLE_VALUE)
{
    strcpy (portName, "COM1");
    errorMessage[0] = 0;
}

//==========================================================================
// Remove of Object
//==========================================================================
CSerial::~CSerial (void)
{
    closePort ();
}


//==========================================================================
// Search config string input by user
//  Return the pointer of the *str start point inside *cfg
//==========================================================================
const char *CSerial::searchCfg (const char *aCfg, const char *aStr)
{
    return strstr (aCfg, aStr);
}

//==========================================================================
// Open Port
//==========================================================================
int CSerial::openPort (const char *aPortName, const char *aCfg)
{
    int i;
    unsigned long flag_baud;
    unsigned long flag_mode;
    struct termios new_tio;
    char buf[128];

    // Clear error message
    errorMessage[0] = 0;

    // Default baud and mode
    flag_baud = B9600;
    flag_mode = CS8;

    // Save port name
    strncpy (portName, aPortName, sizeof (portName) - 1);

    // Open Port
    if (fdComPort == INVALID_HANDLE_VALUE)
    {
        fdComPort = open (portName, O_RDWR | O_NONBLOCK, 0);
    }
    else
    {
        strncpy (errorMessage, "CSerial: Already opened.", sizeof (errorMessage) - 1);
        return -1;
    }

    if (fdComPort == INVALID_HANDLE_VALUE)
    {
        strerror_r (errno, buf, sizeof (buf));
        snprintf (errorMessage, sizeof (errorMessage), "CSerial: %s", buf);
        return -1;
    }

    // Save Term
    gOldTio = new struct termios;
    tcgetattr (fdComPort, gOldTio);

    // Extract Baud
    i = 0;
    while (KBaudTable[i].Str != NULL)
    {
        if (searchCfg (aCfg, KBaudTable[i].Str) != NULL)
        {
            baudRate = KBaudTable[i].BaudShow;
            flag_baud = KBaudTable[i].BaudValue;
            break;
        }
        i++;
    }

    // Extract Mode
    i = 0;
    while (KModeTable[i].Str != NULL)
    {
        if (searchCfg (aCfg, KModeTable[i].Str) != NULL)
        {
            mode = KModeTable[i].Str;
            flag_mode = KModeTable[i].ModeValue;
            break;
        }
        i++;
    }

    // Set Term
    memset (&new_tio, 0, sizeof (new_tio));
    new_tio.c_cflag = flag_baud | flag_mode | CLOCAL | CREAD;
    new_tio.c_iflag = IGNPAR;
    new_tio.c_oflag = 0;
    new_tio.c_lflag = 0;
    new_tio.c_cc[VTIME] = 0;
    new_tio.c_cc[VMIN] = 0;
    
    tcflush (fdComPort, TCIOFLUSH);
    tcsetattr (fdComPort, TCSANOW, &new_tio);

    return 0;
}

//==========================================================================
// Close Port
//==========================================================================
void CSerial::closePort (void)
{
    // Close COM
    if (fdComPort != INVALID_HANDLE_VALUE)
    {
        tcflush (fdComPort, TCIOFLUSH);
        if (gOldTio != NULL)
        {
            tcsetattr (fdComPort, TCSANOW, gOldTio);
            delete gOldTio;
            gOldTio = NULL;
        }
        close (fdComPort);
    }
    fdComPort = INVALID_HANDLE_VALUE;
}

//==========================================================================
// Check Port opened
//==========================================================================
bool CSerial::isOpened (void)
{
    if (fdComPort == INVALID_HANDLE_VALUE)
        return false;
    else
        return true;
}

//==========================================================================
// Get RXed data
//==========================================================================
int CSerial::readPort (unsigned char *aDest, int aDestLen)
{
    int ret;
    char buf[128];

    errorMessage[0] = 0;

    if (fdComPort == INVALID_HANDLE_VALUE)
    {
        strncpy (errorMessage, "CSerial: Port not open yet.", sizeof (errorMessage) - 1);
        return -1;
    }

    ret = read (fdComPort, aDest, aDestLen);
    if (ret < 0)
    {
        strerror_r (errno, buf, sizeof (buf));
        snprintf (errorMessage, sizeof (errorMessage), "CSerial: %s", buf);
        return -1;
    }
    else
        return ret;
}

//==========================================================================
// Send data
//==========================================================================
int CSerial::writePort (const unsigned char *aSrc, int aSrcLen)
{
    int ret;
    char buf[128];

    errorMessage[0] = 0;

    if (fdComPort == INVALID_HANDLE_VALUE)
    {
        strncpy (errorMessage, "CSerial: Port not open yet.", sizeof (errorMessage) - 1);
        return -1;
    }

    ret = write (fdComPort, aSrc, aSrcLen);
    if (ret < 0)
    {
        strerror_r (errno, buf, sizeof (buf));
        snprintf (errorMessage, sizeof (errorMessage), "CSerial: %s", buf);
        return -1;
    }
    else
        return ret;
}

//==========================================================================
// Send Null-Terminal Sting
//==========================================================================
int CSerial::putsToPort (const char *aSrc)
{
    int src_len;
    int ret;
    char buf[128];

    errorMessage[0] = 0;

    if (fdComPort == INVALID_HANDLE_VALUE)
    {
        strncpy (errorMessage, "CSerial: Port not open yet.", sizeof (errorMessage) - 1);
        return -1;
    }

    src_len = (int)strlen (aSrc);
    ret = write (fdComPort, aSrc, src_len);
    if (ret < 0)
    {
        strerror_r (errno, buf, sizeof (buf));
        snprintf (errorMessage, sizeof (errorMessage), "CSerial: %s", buf);
        return -1;
    }
    else
        return ret;
}

//==========================================================================
// Flush buffer
//==========================================================================
void CSerial::flushPort (void)
{
    if (fdComPort == INVALID_HANDLE_VALUE)
        return;
    tcflush (fdComPort, TCIFLUSH);
}

//==========================================================================
// Control DTR signal
//==========================================================================
void CSerial::dtr (bool aState)
{
    int status;

    if (fdComPort == INVALID_HANDLE_VALUE)
        return;

    ioctl (fdComPort, TIOCMGET, &status);
    if (aState)
        status |= TIOCM_DTR;
    else
        status &= (!TIOCM_DTR);
    ioctl (fdComPort, TIOCMSET, &status);
}

//==========================================================================
// Read DSR signal
//==========================================================================
bool CSerial::dsr (void)
{
    int status;

    if (fdComPort == INVALID_HANDLE_VALUE)
        return false;

    ioctl (fdComPort, TIOCMGET, &status);
    if (status & TIOCM_DSR)
        return true;
    else
        return false;
}


//==========================================================================
// Control RTS signal
//==========================================================================
void CSerial::rts (bool aState)
{
    int status;

    if (fdComPort == INVALID_HANDLE_VALUE)
        return;

    ioctl (fdComPort, TIOCMGET, &status);
    if (aState)
        status |= TIOCM_RTS;
    else
        status &= (!TIOCM_RTS);
    ioctl (fdComPort, TIOCMSET, &status);
}

//==========================================================================
// Read CTS signal
//==========================================================================
bool CSerial::cts (void)
{
    int status;

    if (fdComPort == INVALID_HANDLE_VALUE)
        return false;

    ioctl (fdComPort, TIOCMGET, &status);
    if (status & TIOCM_CTS)
        return true;
    else
        return false;
}
