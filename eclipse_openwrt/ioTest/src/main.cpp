//==========================================================================
// GPIO Testing
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <getopt.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>

#include "debug.h"
#include "utils.h"
#include "safe_string.h"
#include "gpio.h"
#include "serial.h"

#include "main.h"

//==========================================================================
// Defines
//==========================================================================

//==========================================================================
// Prototypes
//==========================================================================
static int Go (void);

//==========================================================================
// Constants
//==========================================================================

//==========================================================================
// Variables
//==========================================================================
static int gArgCnt;
static char **gArgList;
static bool gShowVersion = false;

//==========================================================================
// Main Entry
//==========================================================================
int main (int aArgc, char *aArgv[])
{
    bool restore;

    // Non buffered output
    setvbuf (stdout, NULL, _IONBF, 0);


    // Non canonical mode output
    struct termios old_termios;
    struct termios new_termios;

    if (tcgetattr (STDIN_FILENO, &old_termios))
        printf ("tcgetattr fail!\n");
    else
    {
        // Mark to activate restore action before exit
        restore = true;

        // Modify from old attributes
        new_termios = old_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
        new_termios.c_iflag &= ~(ISTRIP | IGNCR | INLCR | IXOFF | IXON);
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 0;

        // Apply new attributes
        if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &new_termios))
            printf ("tcsetattr fail!\n");
    }

    DEBUG_INIT ();

    // Start the main
    gArgCnt = aArgc;
    gArgList = aArgv;

    int ret;
    ret = Go ();

    DEBUG_EXIT ();

    // Restore to old attributes
    if (restore)
    {
        if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &old_termios))
            printf ("tcsetattr fail!\n");
    }

    return ret;
}

//==========================================================================
// Show Program info
//==========================================================================
static void ShowProgramInfo (void)
{
    printf ("%s " STR_VERSION, FileNameOnly (gArgList[0]));
    if (DEBUG)
        printf (" DEBUG");
    printf (" (build on " __DATE__ " " __TIME__ ")");
    printf (" " STR_COPYRIGHT "\n");
}

//==========================================================================
// Show help
//==========================================================================
static void ShowHelp (void)
{
    printf ("\n");
    printf ("Syntax: %s [options]\n", FileNameOnly (gArgList[0]));
    printf ("Options:\n");
    printf ("  -v, --version    Show version.\n");
    printf ("\n");
    printf ("\n");
}

//==========================================================================
// Extract operation parameters from command line
//==========================================================================
static struct option KLongOptions[] =
{
 { "help", 0, NULL, 'h'},
 { "version", 0, NULL, 'v'},
 { NULL, 0, NULL, 0}
};

static int ExtractParam (void)
{
    int i;

    while (1)
    {
        i = getopt_long (gArgCnt, gArgList, "vh", KLongOptions, NULL);
        if (i == EOF)
            break;
        switch (i)
        {
            case 'v':
                gShowVersion = true;
                break;

            default:
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

//==========================================================================
// Get a pressed key from console
//==========================================================================
static int GetKey (int *aKeyCode = NULL)
{
    char buf[1];
    int len;

    len = read (STDIN_FILENO, buf, sizeof (buf));
    if (len > 0)
        return buf[0];
    else
        return 0;
}

//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================

//==========================================================================
// Test all GPIO
//==========================================================================
static int TestAllGpio (void)
{
    int ch;
    int i;
    int loop;
    CGpio gpio;
    unsigned long tick_wait;

    int gpio_list[] = {36, 37, 39, 40, 41, 42, 0, 1, 2, 3, 11,
                       14, 15, 16, 17, 18, 19, -1};

    // Setup all GPIO
    i = 0;
    while (gpio_list[i] >= 0)
    {
        printf ("Setup GPIO%d\n", gpio_list[i]);
        if (gpio.setup (gpio_list[i], true) < 0)
        {
            printf ("ERROR. %s\n", gpio.errorMessage);
            return -1;
        }
        i ++;
    }

    for (loop = 0; loop < 2; loop ++)
    {
        // Turn on all GPIO
        i = 0;
        while (gpio_list[i] >= 0)
        {
            if (gpio.set (gpio_list[i], GPIO_STATE_HIGH) < 0)
            {
                printf ("ERROR. %s\n", gpio.errorMessage);
                return -1;
            }
            i ++;
        }

        // Wait 1s
        tick_wait = GetTick ();
        while (TickElapsed (tick_wait) < 1000)
        {
            ch = GetKey ();
            if (ch == 27)
            {
                printf ("ABORT\n");
                return -1;
            }
            usleep (10000);
        }

        // Turn off all GPIO
        i = 0;
        while (gpio_list[i] >= 0)
        {
            if (gpio.set (gpio_list[i], GPIO_STATE_LOW) < 0)
            {
                printf ("ERROR. %s\n", gpio.errorMessage);
                return -1;
            }
            i ++;
        }

        // Wait 1s
        tick_wait = GetTick ();
        while (TickElapsed (tick_wait) < 1000)
        {
            ch = GetKey ();
            if (ch == 27)
            {
                printf ("ABORT\n");
                return -1;
            }
            usleep (10000);
        }
    }

    // Lit the testing LED once per GPIO
    i = 0;
    while (gpio_list[i] >= 0)
    {
        printf ("Test GPIO%d\n", gpio_list[i]);

        // Turn on LED
        if (gpio.set (gpio_list[i], GPIO_STATE_HIGH) < 0)
        {
            printf ("ERROR. %s\n", gpio.errorMessage);
            return -1;
        }

        // Wait
        tick_wait = GetTick ();
        while (TickElapsed (tick_wait) < 500)
        {
            ch = GetKey ();
            if (ch == 27)
            {
                printf ("ABORT\n");
                return -1;
            }
            usleep (10000);
        }

        // Turn off LED
        if (gpio.set (gpio_list[i], GPIO_STATE_LOW) < 0)
        {
            printf ("ERROR. %s\n", gpio.errorMessage);
            return -1;
        }

        // Wait
        tick_wait = GetTick ();
        while (TickElapsed (tick_wait) < 500)
        {
            ch = GetKey ();
            if (ch == 27)
            {
                printf ("ABORT\n");
                return -1;
            }
            usleep (10000);
        }

        i ++;
    }

    return 0;
}

//==========================================================================
// Test UART0 and UART1
//==========================================================================
static int TestUart (void)
{
    CSerial uart0;
    CSerial uart1;
    unsigned char tx_buf[4096];
    unsigned char rx_buf[5120];
    unsigned char buf[128];
    int rxing_idx;
    int i;
    int ret;
    unsigned long tick_rx;
    int loop;

    // Open UARTs
    if (uart0.openPort ("/dev/ttyS0", "115200 N81") < 0)
    {
        printf ("UART0 ERROR. %s\n", uart0.errorMessage);
        return -1;
    }
    if (uart1.openPort ("/dev/ttyS1", "115200 N81") < 0)
    {
        printf ("UART1 ERROR. %s\n", uart1.errorMessage);
        return -1;
    }

    printf ("UART0 and UART1 opened.\n");

    // Flush data
    uart0.flushPort ();
    uart1.flushPort ();

    //
    for (loop = 0; loop < 5; loop ++)
    {

        // Random tx_buf and clear rx_buf
        for (i = 0; i < (int)sizeof (tx_buf); i ++)
            tx_buf[i] = rand();
        memset (rx_buf, 0, sizeof (rx_buf));

        // Write to UART0
        ret = uart0.writePort (tx_buf, sizeof (tx_buf));
        if (ret < 0)
        {
            printf ("UART0 ERROR. %s\n", uart0.errorMessage);
            return -1;
        }
        else if (ret != sizeof (tx_buf))
        {
            printf ("UART0 writePort return length incorrect.\n");
            return -1;
        }
        printf ("UART0 %d bytes sent.\n", ret);

        // Get data from UART1
        tick_rx = GetTick ();
        rxing_idx = 0;
        while (rxing_idx < (int)sizeof (tx_buf))
        {
            ret = uart1.readPort (buf, sizeof (buf));
            if (ret < 0)
            {
                printf ("UART1 ERROR. %s\n", uart1.errorMessage);
                return -1;
            }
            if (ret == 0)
            {
                if (TickElapsed (tick_rx) > 1000)
                {
                    printf ("UART1 ERROR. Rx timeout\n");
                    return -1;
                }
                // no data
                usleep (1000);
                continue;
            }
            if ((rxing_idx + ret) > (int)sizeof (rx_buf))
            {
                printf ("UART1 ERROR. Rxed data too large.\n");
                return -1;
            }
            memcpy (&rx_buf[rxing_idx], buf, ret);
            rxing_idx += ret;
            tick_rx = GetTick ();
        }
        printf ("UART1 received %d bytes.\n", rxing_idx);

        // Verify data
        if (memcmp (tx_buf, rx_buf, sizeof (tx_buf)) != 0)
        {
            printf ("UART0 to UART1 data verify error.\n");
            return -1;
        }

        // Check UART0 nothing received
        ret = uart0.readPort (rx_buf, sizeof (rx_buf));
        if (ret < 0)
        {
            printf ("UART0 ERROR. %s\n", uart0.errorMessage);
            return -1;
        }
        if (ret > 0)
        {
            printf ("UART0 received unexpected data.\n");
            return -1;
        }

        //
        printf ("UART0 to UART1 transfer test %d PASSED.\n", loop + 1);

        // Random tx_buf and clear rx_buf
        for (i = 0; i < (int)sizeof (tx_buf); i ++)
            tx_buf[i] = rand();
        memset (rx_buf, 0, sizeof (rx_buf));

        // Write to UART1
        ret = uart1.writePort (tx_buf, sizeof (tx_buf));
        if (ret < 0)
        {
            printf ("UART1 ERROR. %s\n", uart1.errorMessage);
            return -1;
        }
        else if (ret != sizeof (tx_buf))
        {
            printf ("UART1 writePort return length incorrect.\n");
            return -1;
        }
        printf ("UART1 %d bytes sent.\n", ret);

        // Get data from UART0
        tick_rx = GetTick ();
        rxing_idx = 0;
        while (rxing_idx < (int)sizeof (tx_buf))
        {
            ret = uart0.readPort (buf, sizeof (buf));
            if (ret < 0)
            {
                printf ("UART0 ERROR. %s\n", uart0.errorMessage);
                return -1;
            }
            if (ret == 0)
            {
                if (TickElapsed (tick_rx) > 1000)
                {
                    printf ("UART0 ERROR. Rx timeout\n");
                    return -1;
                }
                // no data
                usleep (1000);
                continue;
            }
            if ((rxing_idx + ret) > (int)sizeof (rx_buf))
            {
                printf ("UART0 ERROR. Rxed data too large.\n");
                return -1;
            }
            memcpy (&rx_buf[rxing_idx], buf, ret);
            rxing_idx += ret;
            tick_rx = GetTick ();
        }
        printf ("UART0 received %d bytes.\n", rxing_idx);

        // Verify data
        if (memcmp (tx_buf, rx_buf, sizeof (tx_buf)) != 0)
        {
            printf ("UART1 to UART0 data verify error.\n");
            return -1;
        }

        // Check UART1 nothing received
        ret = uart1.readPort (rx_buf, sizeof (rx_buf));
        if (ret < 0)
        {
            printf ("UART1 ERROR. %s\n", uart0.errorMessage);
            return -1;
        }
        if (ret > 0)
        {
            printf ("UART1 received unexpected data.\n");
            return -1;
        }

        //
        printf ("UART1 to UART0 transfer test %d PASSED.\n", loop + 1);

    }

    // Close UARTs
    uart0.closePort ();
    uart1.closePort ();
    printf ("UART0 and UART1 closed.\n");
    return 0;

}


//==========================================================================
// Start of process
//==========================================================================
static int Go (void)
{
    time_t now;


    // Get Time and initialize random
    time (&now);
    tzset ();
    srand (now);

    // Extract cmd-line parameter
    if (ExtractParam () == EXIT_FAILURE)
    {
        ShowProgramInfo ();
        ShowHelp ();
        return EXIT_FAILURE;
    }

    // Show version
    if (gShowVersion)
    {
        ShowProgramInfo ();
        return EXIT_SUCCESS;
    }

    //
    if (TestUart () < 0)
        return EXIT_FAILURE;

    if (TestAllGpio () < 0)
        return EXIT_FAILURE;

    //
    return EXIT_SUCCESS;
}
