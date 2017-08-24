//==========================================================================
//==========================================================================
#ifndef _INC_GPIO_H
#define _INC_GPIO_H

//==========================================================================
//==========================================================================

//==========================================================================
// Defines
//==========================================================================
#define NUM_OF_GPIO         64

#define GPIO_STATE_UNKNOWN  -1
#define GPIO_STATE_LOW      0
#define GPIO_STATE_HIGH     1

struct TGpioInfo
{
    int fd;
    int state;
    bool isOutput;
};

//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
class CGpio
{
public:
    CGpio (void);
    ~CGpio (void);

    int setup (int aGpioNumber, bool asOutput = true);
    int set (int aGpioNumber, int aState);
    int get (int aGpioNumber);

    char errorMessage[128];

private:

    struct TGpioInfo gpioInfo[NUM_OF_GPIO];
};

//==========================================================================
//==========================================================================
#endif
