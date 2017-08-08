//==========================================================================
// Rule Handling
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

#include "main.h"

//==========================================================================
// Defines
//==========================================================================
#define PATH_I2C_PORT       "/dev/i2c-0"
#define PATH_MTD_FACTORY    "/dev/mtd2"
#define PATH_OUTPUT_FILE    "/tmp/mtd2.bin"

#define SIZE_MTD_FACTORY    65536

#define MTD_OFFSET_LAN_MAC  0x28
#define MTD_OFFSET_WIFI_MAC 0x04

#define RANDOM_MAC_ETH_PREFIX   0xC2C2C2
#define RANDOM_MAC_WIFI_PREFIX  0xC2C2C1


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

static unsigned char gEepromMacAddr1[6];
static unsigned char gEepromMacAddr2[6];

static unsigned char gMtdContent[SIZE_MTD_FACTORY];

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
//==========================================================================
//==========================================================================
//==========================================================================

//==========================================================================
// Read MAC Address from EEPROM
//==========================================================================
static int ReadEepromMacAddr (void)
{
    int fd;
    int ret;
    unsigned l;

    //
    memset (gEepromMacAddr1, 0xff, sizeof (gEepromMacAddr1));
    memset (gEepromMacAddr2, 0xff, sizeof (gEepromMacAddr2));

    //
    fd = open (PATH_I2C_PORT, O_RDWR);
    if (fd < 0)
    {
        perror ("readMacAddres() open");
        return -1;
    }

    ret = -1;
    do
    {
        unsigned char buf[8];
        unsigned char addr = 0xfa;

        // Read 1st chip
        if (ioctl (fd,I2C_SLAVE, (0xA0 >> 1)) < 0)
        {
            perror ("readMacAddres() ioctl");
            break;
        }

        if (write (fd, &addr, 1) < 0)
        {
            perror ("readMacAddres() write");
            break;
        }

        if (read (fd, buf, 6) < 0)
        {
            perror ("readMacAddres() read");
            break;
        }

        DEBUG_PRINTF ("EEPROM READ");
        DEBUG_HEX2STRING (buf, 6);

        // Check Mircochip MAC Address OUI
        l = ((buf[0] << 16) | (buf[1] << 8) | (buf[2] << 0));
        if ((l == 0xD88039) || (l == 0x001EC0) || (l == 0x0004A3))
        {
            memcpy (gEepromMacAddr1, buf, 6);
        }

        // Read 2nd chip
        if (ioctl (fd,I2C_SLAVE, (0xA2 >> 1)) < 0)
        {
            perror ("readMacAddres() ioctl");
            break;
        }

        if (write (fd, &addr, 1) < 0)
        {
            perror ("readMacAddres() write");
            break;
        }

        if (read (fd, buf, 6) < 0)
        {
            perror ("readMacAddres() read");
            break;
        }

        DEBUG_PRINTF ("EEPROM READ");
        DEBUG_HEX2STRING (buf, 6);

        // Check Mircochip MAC Address OUI
        l = ((buf[0] << 16) | (buf[1] << 8) | (buf[2] << 0));
        if ((l == 0xD88039) || (l == 0x001EC0) || (l == 0x0004A3))
        {
            memcpy (gEepromMacAddr2, buf, 6);
        }


        //
        ret = 0;

    } while (0);

    close (fd);

    return ret;
}

//==========================================================================
// Read Factory setting MTD
//==========================================================================
static int ReadMtd (void)
{
    int fd;
    int ret;
    size_t mtd_size;

    fd = open (PATH_MTD_FACTORY, O_RDONLY);
    if (fd < 0)
    {
        perror ("Open MTD");
        return -1;
    }

    ret = -1;
    do
    {
        //
        mtd_size = lseek (fd, 0, SEEK_END);
        printf ("MTD size %d bytes.\n", mtd_size);
        if (mtd_size != SIZE_MTD_FACTORY)
        {
            printf ("Invalid MTD size.\n");
            return -1;
        }

        //
        lseek (fd, 0, SEEK_SET);
        if (read (fd, gMtdContent, SIZE_MTD_FACTORY) != SIZE_MTD_FACTORY)
        {
            printf ("Read MTD error.\n");
            return -1;
        }

        // All done
        ret = 0;
    } while (0);

    close (fd);

    //
    return ret;
}


//==========================================================================
// Write output file
//==========================================================================
static int WriteOutput (void)
{
    int fd;

    fd = open (PATH_OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd < 0)
    {
        perror ("Write Output File");
        return -1;
    }
    else
    {
        if (write (fd, gMtdContent, SIZE_MTD_FACTORY) != SIZE_MTD_FACTORY)
        {
            printf ("Write output file fail.\n");
            close (fd);
            return -1;
        }
        close (fd);

        printf ("%s write success.\n", PATH_OUTPUT_FILE);
    }

    return 0;
}


//==========================================================================
// Start of process
//==========================================================================
static int Go (void)
{
    time_t now;
    unsigned long l;
    unsigned char rand_mac1[6];
    unsigned char rand_mac2[6];


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

    // Read EEPROM
    if (ReadEepromMacAddr () < 0)
    {
        return EXIT_FAILURE;
    }
    printf ("EEPROM read success\n");
    printf ("EEPROM MAC1: %02X:%02X:%02X:%02X:%02X:%02X\n",
            gEepromMacAddr1[0], gEepromMacAddr1[1], gEepromMacAddr1[2],
            gEepromMacAddr1[3], gEepromMacAddr1[4], gEepromMacAddr1[5]);
    printf ("EEPROM MAC2: %02X:%02X:%02X:%02X:%02X:%02X\n",
            gEepromMacAddr2[0], gEepromMacAddr2[1], gEepromMacAddr2[2],
            gEepromMacAddr2[3], gEepromMacAddr2[4], gEepromMacAddr2[5]);

    if (ReadMtd () < 0)
    {
        return EXIT_FAILURE;
    }

    // Gen random EEPROM MAC Address
    time (&now);
    srand (now);
    l = rand ();

    rand_mac1[0] = (unsigned char)(RANDOM_MAC_ETH_PREFIX >> 16);
    rand_mac1[1] = (unsigned char)(RANDOM_MAC_ETH_PREFIX >> 8);
    rand_mac1[2] = (unsigned char)(RANDOM_MAC_ETH_PREFIX >> 0);
    rand_mac1[3] = (unsigned char)(l >> 16);
    rand_mac1[4] = (unsigned char)(l >> 8);
    rand_mac1[5] = (unsigned char)(l >> 0);

    rand_mac2[0] = (unsigned char)(RANDOM_MAC_WIFI_PREFIX >> 16);
    rand_mac2[1] = (unsigned char)(RANDOM_MAC_WIFI_PREFIX >> 8);
    rand_mac2[2] = (unsigned char)(RANDOM_MAC_WIFI_PREFIX >> 0);
    rand_mac2[3] = (unsigned char)(l >> 16);
    rand_mac2[4] = (unsigned char)(l >> 8);
    rand_mac2[5] = (unsigned char)(l >> 0);

    if (memcmp (gEepromMacAddr1, "\xff\xff\xff\xff\xff\xff", 6) == 0)
    {
        printf ("!!EEPROM MAC1 is invalid, using random MAC address instead.\n");
        memcpy (gEepromMacAddr1, rand_mac1, sizeof (gEepromMacAddr1));
        printf ("!!MAC1 Changed to: %02X:%02X:%02X:%02X:%02X:%02X\n",
                gEepromMacAddr1[0], gEepromMacAddr1[1], gEepromMacAddr1[2],
                gEepromMacAddr1[3], gEepromMacAddr1[4], gEepromMacAddr1[5]);
    }

    if (memcmp (gEepromMacAddr2, "\xff\xff\xff\xff\xff\xff", 6) == 0)
    {
        printf ("!!EEPROM MAC1 is invalid, using random MAC address instead.\n");
        memcpy (gEepromMacAddr2, rand_mac2, sizeof (gEepromMacAddr2));
        printf ("!!MAC1 Changed to: %02X:%02X:%02X:%02X:%02X:%02X\n",
                gEepromMacAddr2[0], gEepromMacAddr2[1], gEepromMacAddr2[2],
                gEepromMacAddr2[3], gEepromMacAddr2[4], gEepromMacAddr2[5]);
    }

    // Update MAC to MTD content
    memcpy (&gMtdContent[MTD_OFFSET_LAN_MAC], gEepromMacAddr1, sizeof (gEepromMacAddr1));
    memcpy (&gMtdContent[MTD_OFFSET_WIFI_MAC], gEepromMacAddr2, sizeof (gEepromMacAddr2));

    // Write signature
    if (memcmp (&gMtdContent[0], "\x28\x76", 2))
    {
        memcpy (&gMtdContent[0], "\x28\x76\x00\x02", 4);
    }

    //
    if (WriteOutput () < 0)
        return EXIT_FAILURE;


    //
    printf ("Use the following command to update MAC address.\n");
    printf ("    mtd write %s factory\n", PATH_OUTPUT_FILE);
    printf ("Then run the following command to init the system with new MAC address.\n");
    printf ("    /sbin/firstboot\n");
    printf ("After that, reboot the system.\n");

    return EXIT_SUCCESS;
}
