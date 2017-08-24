//==========================================================================
// GPIOs
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "utils.h"
#include "debug.h"
#include "gpio.h"
#include "safe_string.h"


//==========================================================================
// Defines
//==========================================================================
#define PATH_GPIO_CLASS             "/sys/class/gpio"
#define PATH_GPIO_EXPORT            PATH_GPIO_CLASS"/export"
#define FILENAME_GPIO_DIRECTION     "direction"
#define FILENAME_GPIO_VALUE         "value"

//==========================================================================
//==========================================================================
// Private members
// vvvvvvvvvvvvvv
//==========================================================================
//==========================================================================


//==========================================================================
//==========================================================================
// ^^^^^^^^^^^^^^^
// Private members
//==========================================================================
//==========================================================================
// Public members
// vvvvvvvvvvvvvv
//==========================================================================
//==========================================================================

//==========================================================================
// Constructor
//==========================================================================
CGpio::CGpio (void)
{
    int i;

    // Mark all GPIO not opened.
    for (i = 0; i < NUM_OF_GPIO; i ++)
    {
        gpioInfo[i].fd = -1;
    }
}

//==========================================================================
// Destroyer
//==========================================================================
CGpio::~CGpio (void)
{
    int i;

    // Close opened GPIOs
    for (i = 0; i < NUM_OF_GPIO; i ++)
    {
        if (gpioInfo[i].fd >= 0)
        {
            close (gpioInfo[i].fd);
            gpioInfo[i].fd = -1;
        }
    }
}

//==========================================================================
// Setup GPIO
//==========================================================================
int CGpio::setup (int aGpioNumber, bool asOutput)
{
    int fd;
    char buf[128];
    char path_gpio[128];

    // Clear last error message
    errorMessage[0] = 0;

    // Check GPIO number range
    if ((aGpioNumber < 0) || (aGpioNumber >= NUM_OF_GPIO))
    {
        snprintf (errorMessage, sizeof (errorMessage), "GPIO number out range, 0 to %d.", NUM_OF_GPIO);
        return -1;
    }

    // Check GPIO opened
    if (gpioInfo[aGpioNumber].fd >= 0)
    {
        return 0;
    }

    // Check GPIO exported
    snprintf (path_gpio, sizeof (path_gpio), "%s/gpio%d", PATH_GPIO_CLASS, aGpioNumber);
    if (access (path_gpio, F_OK) < 0)
    {
        fd = open (PATH_GPIO_EXPORT, O_WRONLY);
        if (fd < 0)
        {
            strerror_r (errno, buf, sizeof (buf));
            snprintf (errorMessage, sizeof (errorMessage), "Export GPIO%d %s", aGpioNumber, buf);
            return -1;
        }
        else
        {
            snprintf (buf, sizeof (buf), "%d", aGpioNumber);
            write (fd, buf, strlen (buf));
            close (fd);
            DEBUG_PRINTF ("Export GPIO%d success.", aGpioNumber);
        }
    }
    else
    {
    }

    // Setup GPIO
    snprintf (path_gpio, sizeof (path_gpio), "%s/gpio%d/%s", PATH_GPIO_CLASS, aGpioNumber, FILENAME_GPIO_DIRECTION);
    fd = open (path_gpio, O_WRONLY);
    if (fd < 0)
    {
        strerror_r (errno, buf, sizeof (buf));
        snprintf (errorMessage, sizeof (errorMessage), "Fail to open %s, %s", path_gpio, buf);
        return -1;
    }
    else
    {
        if (asOutput)
        {
            write (fd, "out", 3);
            DEBUG_PRINTF ("Set GPIO%d as output", aGpioNumber);
        }
        else
        {
            write (fd, "in", 2);
            DEBUG_PRINTF ("Set GPIO%d as input", aGpioNumber);
        }
        close (fd);
    }

    // Open GPIO value
    snprintf (path_gpio, sizeof (path_gpio), "%s/gpio%d/%s", PATH_GPIO_CLASS, aGpioNumber, FILENAME_GPIO_VALUE);
    if (asOutput)
        fd = open (path_gpio, O_WRONLY);
    else
        fd = open (path_gpio, O_RDONLY);
    if (fd < 0)
    {
        strerror_r (errno, buf, sizeof (buf));
        snprintf (errorMessage, sizeof (errorMessage), "Fail to open %s, %s", path_gpio, buf);
        return -1;
    }

    // Open success
    DEBUG_PRINTF ("%s opened", path_gpio);
    gpioInfo[aGpioNumber].fd = fd;
    gpioInfo[aGpioNumber].state = GPIO_STATE_UNKNOWN;
    gpioInfo[aGpioNumber].isOutput = asOutput;

    return 0;
}

//==========================================================================
// Set GPIO state
//==========================================================================
int CGpio::set (int aGpioNumber, int aState)
{
    // Clear last error message
    errorMessage[0] = 0;

    // Check GPIO number range
    if ((aGpioNumber < 0) || (aGpioNumber >= NUM_OF_GPIO))
    {
        snprintf (errorMessage, sizeof (errorMessage), "GPIO number out range, 0 to %d.", NUM_OF_GPIO);
        return -1;
    }

    // Set GPIO opened
    if (gpioInfo[aGpioNumber].fd < 0)
    {
        snprintf (errorMessage, sizeof (errorMessage), "GPIO%d not opened.", aGpioNumber);
        return -1;
    }

    // Write to GPIO value
    if (gpioInfo[aGpioNumber].isOutput)
    {
        if (gpioInfo[aGpioNumber].state != aState)
        {
            if (aState == GPIO_STATE_LOW)
            {
                gpioInfo[aGpioNumber].state = GPIO_STATE_LOW;
                write (gpioInfo[aGpioNumber].fd, "0", 1);
            }
            else if (aState == GPIO_STATE_HIGH)
            {
                gpioInfo[aGpioNumber].state = GPIO_STATE_HIGH;
                write (gpioInfo[aGpioNumber].fd, "1", 1);
            }
        }
        return 0;
    }
    else
    {
        snprintf (errorMessage, sizeof (errorMessage), "Unable to write GPIO%d, it is input.", aGpioNumber);
        return -1;
    }
}

//==========================================================================
// Get GPIO state
//==========================================================================
int CGpio::get (int aGpioNumber)
{
    int ret;
    char buf[128];

    // Clear last error message
    errorMessage[0] = 0;

    // Check GPIO number range
    if ((aGpioNumber < 0) || (aGpioNumber >= NUM_OF_GPIO))
    {
        snprintf (errorMessage, sizeof (errorMessage), "GPIO number out range, 0 to %d.", NUM_OF_GPIO);
        return -1;
    }

    // Set GPIO opened
    if (gpioInfo[aGpioNumber].fd < 0)
    {
        snprintf (errorMessage, sizeof (errorMessage), "GPIO%d not opened.", aGpioNumber);
        return -1;
    }

    // Read from GPIO value
    if (gpioInfo[aGpioNumber].isOutput == false)
    {
        memset (buf, 0, sizeof (buf));
        lseek (gpioInfo[aGpioNumber].fd, 0, SEEK_SET);
        ret = read (gpioInfo[aGpioNumber].fd, buf, sizeof (buf) - 1);
        if (ret < 0)
        {
            strerror_r (errno, buf, sizeof (buf));
            snprintf (errorMessage, sizeof (errorMessage), "Unable to read GPIO%d, %s", aGpioNumber, buf);
            gpioInfo[aGpioNumber].state = GPIO_STATE_UNKNOWN;
        }
        if (buf[0] == '0')
            gpioInfo[aGpioNumber].state = GPIO_STATE_LOW;
        else if (buf[0] == '1')
            gpioInfo[aGpioNumber].state = GPIO_STATE_HIGH;
        else
        {
            snprintf (errorMessage, sizeof (errorMessage), "Invalid value read from GPIO%d.", aGpioNumber);
            gpioInfo[aGpioNumber].state = GPIO_STATE_UNKNOWN;
        }
    }

    //
    return gpioInfo[aGpioNumber].state;
}
