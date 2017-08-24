//==================================================================
// Serial Port Class
//  Using Windows API to access
//==================================================================
#ifndef _INC_SERIAL_H
#define _INC_SERIAL_H

//==================================================================
//==================================================================
//#include <windows.h>
#define HANDLE      int

//==================================================================
// Defines
//==================================================================
#ifndef true
#define true    1
#endif
#ifndef false
#define false   0
#endif

//==================================================================
// Class defines
//==================================================================
class CSerial
{
public:
    CSerial (void);                                         // Creation
    ~CSerial (void);                                        // Removal
    int openPort (const char *aPortName, const char *aCfg);     // Open
    void closePort (void);                                      // Close Port
    bool isOpened (void);                                   // Check Port opened
    int readPort (unsigned char *aDest, int aDestLen);          // Read data
    int writePort (const unsigned char *aSrc, int aSrcLen);     // Send data
    int putsToPort (const char *aSrc);                            // Send String
    void flushPort (void);                                      // Flush buffer
    void dtr (bool aState);                                 // Set DTR state
    bool dsr (void);                                        // Read DSR state
    void rts (bool aState);                                 // Set RTS state
    bool cts (void);                                        // Read CTS state

    char portName[64];                                      // Port Name
    int baudRate;                                           // Port baud
    const char *mode;                                       // Port mode str
    char errorMessage[128];                                 // Error Message

private:
    HANDLE fdComPort;                                       // COM Port Fid
    const char *searchCfg (const char *aCfg, const char *aStr);

};

//==================================================================
//==================================================================
#endif
