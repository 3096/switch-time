/* Copyright (c) 2024 switchtime Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "ntp.h"

bool setsysInternetTimeSyncIsOn() {
    Result rs = setsysInitialize();
    if (R_FAILED(rs)) {
        printf("setsysInitialize failed, %x\n", rs);
        return false;
    }

    bool internetTimeSyncIsOn;
    rs = setsysIsUserSystemClockAutomaticCorrectionEnabled(&internetTimeSyncIsOn);
    setsysExit();
    if (R_FAILED(rs)) {
        printf("Unable to detect if Internet time sync is enabled, %x\n", rs);
        return false;
    }

    return internetTimeSyncIsOn;
}

Result enableSetsysInternetTimeSync() {
    Result rs = setsysInitialize();
    if (R_FAILED(rs)) {
        printf("setsysInitialize failed, %x\n", rs);
        return rs;
    }

    rs = setsysSetUserSystemClockAutomaticCorrectionEnabled(true);
    setsysExit();
    if (R_FAILED(rs)) {
        printf("Unable to enable Internet time sync: %x\n", rs);
    }

    return rs;
}

/*

Type   | SYNC | User | Local | Network
=======|======|======|=======|========
User   |      |      |       |
-------+------+------+-------+--------
Menu   |      |  *   |   X   |
-------+------+------+-------+--------
System |      |      |       |   X
-------+------+------+-------+--------
User   |  ON  |      |       |
-------+------+------+-------+--------
Menu   |  ON  |      |       |
-------+------+------+-------+--------
System |  ON  |  *   |   *   |   X

*/
TimeServiceType __nx_time_service_type = TimeServiceType_System;
bool setNetworkSystemClock(time_t time) {
    Result rs = timeSetCurrentTime(TimeType_NetworkSystemClock, (uint64_t)time);
    if (R_FAILED(rs)) {
        printf("timeSetCurrentTime failed with %x\n", rs);
        return false;
    }
    printf("Successfully set NetworkSystemClock.\n");
    return true;
}

int consoleExitWithMsg(char* msg, PadState* pad) {
    printf("%s\n\nPress + to quit...", msg);

    while (appletMainLoop()) {
        padUpdate(pad);
        u64 kDown = padGetButtonsDown(pad);

        if (kDown & HidNpadButton_Plus) {
            consoleExit(NULL);
            return 0;  // return to hbmenu
        }

        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return 0;
}

bool toggleHBMenuPath(char* curPath, PadState* pad) {
    const char* HB_MENU_NRO_PATH = "sdmc:/hbmenu.nro";
    const char* HB_MENU_BAK_PATH = "sdmc:/hbmenu.nro.bak";
    const char* DEFAULT_RESTORE_PATH = "sdmc:/switch/switch-time.nro";

    printf("\n\n");

    Result rs;
    if (strcmp(curPath, HB_MENU_NRO_PATH) == 0) {
        // restore hbmenu
        rs = access(HB_MENU_BAK_PATH, F_OK);
        if (R_FAILED(rs)) {
            printf("could not find %s to restore. failed: 0x%x", HB_MENU_BAK_PATH, rs);
            consoleExitWithMsg("", pad);
            return false;
        }

        rs = rename(curPath, DEFAULT_RESTORE_PATH);
        if (R_FAILED(rs)) {
            printf("fsFsRenameFile(%s, %s) failed: 0x%x", curPath, DEFAULT_RESTORE_PATH, rs);
            consoleExitWithMsg("", pad);
            return false;
        }
        rs = rename(HB_MENU_BAK_PATH, HB_MENU_NRO_PATH);
        if (R_FAILED(rs)) {
            printf("fsFsRenameFile(%s, %s) failed: 0x%x", HB_MENU_BAK_PATH, HB_MENU_NRO_PATH, rs);
            consoleExitWithMsg("", pad);
            return false;
        }
    } else {
        // replace hbmenu
        rs = rename(HB_MENU_NRO_PATH, HB_MENU_BAK_PATH);
        if (R_FAILED(rs)) {
            printf("fsFsRenameFile(%s, %s) failed: 0x%x", HB_MENU_NRO_PATH, HB_MENU_BAK_PATH, rs);
            consoleExitWithMsg("", pad);
            return false;
        }
        rs = rename(curPath, HB_MENU_NRO_PATH);
        if (R_FAILED(rs)) {
            printf("fsFsRenameFile(%s, %s) failed: 0x%x", curPath, HB_MENU_NRO_PATH, rs);
            rename(HB_MENU_BAK_PATH, HB_MENU_NRO_PATH);  // hbmenu already moved, try to move it back
            consoleExitWithMsg("", pad);
            return false;
        }
    }

    printf("Quick launch toggled\n\n");

    return true;
}

int main(int argc, char* argv[]) {
    consoleInit(NULL);
    printf("SwitchTime v0.1.6\n\n");

    padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeAny(&pad);

    if (!setsysInternetTimeSyncIsOn()) {
        // printf("Trying setsysSetUserSystemClockAutomaticCorrectionEnabled...\n");
        // if (R_FAILED(enableSetsysInternetTimeSync())) {
        //     return consoleExitWithMsg("Internet time sync is not enabled. Please enable it in System Settings.");
        // }
        // doesn't work without rebooting? not worth it
        return consoleExitWithMsg("Internet time sync is not enabled. Please enable it in System Settings.", &pad);
    }

    int arrow_pos[] = {0, 5, 8, 11, 14, 17};
    int arrow_len[] = {4, 2, 2,  2,  2,  2};

    int arrow = 0;

    // Main loop
    while (appletMainLoop()) {
        printf(
            "\n\n"
            "Press:\n\n"
            "UP/DOWN to change by 1   | LEFT/RIGHT to change selection\n"
            "L/ZL to quick decrease   | R/ZR to quick increase\n"
            "A to confirm time        | Y to reset to current time (Cloudflare time server)\n"
            "- to toggle quick launch | + to quit\n\n\n\n\n");

        int yearsChange = 0, monthsChange = 0, daysChange = 0, hoursChange = 0, minutesChange = 0;
        while (appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus) {
                consoleExit(NULL);
                return 0;  // return to hbmenu
            }
            if (kDown & HidNpadButton_Minus) {
                if (!toggleHBMenuPath(argv[0], &pad)) {
                    return 0;
                }
                break;
            }

            time_t currentTime;
            Result rs = timeGetCurrentTime(TimeType_UserSystemClock, (u64*)&currentTime);
            if (R_FAILED(rs)) {
                printf("timeGetCurrentTime failed with %x", rs);
                return consoleExitWithMsg("", &pad);
            }

            struct tm* p_tm_timeToSet = localtime(&currentTime);
            p_tm_timeToSet->tm_year += yearsChange;
            p_tm_timeToSet->tm_mon += monthsChange;
            p_tm_timeToSet->tm_mday += daysChange;
            p_tm_timeToSet->tm_hour += hoursChange;
            p_tm_timeToSet->tm_min += minutesChange;
            time_t timeToSet = mktime(p_tm_timeToSet);

            if (kDown & HidNpadButton_A) {
                printf("\n\n\n");
                setNetworkSystemClock(timeToSet);
                break;
            }

            if (kDown & HidNpadButton_Y) {
                printf("\n\n\n");
                rs = ntpGetTime(&timeToSet);
                if (R_SUCCEEDED(rs)) {
                    setNetworkSystemClock(timeToSet);
                }
                break;
            }

            if (kDown & HidNpadButton_Down) {
                switch (arrow) {
                    case 0: yearsChange--; break;
                    case 1: monthsChange--; break;
                    case 2: daysChange--; break;
                    case 3: hoursChange--; break;
                    case 4: minutesChange--; break;
                }
            } else if (kDown & HidNpadButton_Up) {
                switch (arrow) {
                    case 0: yearsChange++; break;
                    case 1: monthsChange++; break;
                    case 2: daysChange++; break;
                    case 3: hoursChange++; break;
                    case 4: minutesChange++; break;
                }
            } else if (kDown & (HidNpadButton_L | HidNpadButton_ZL)) {
                switch (arrow) {
                    case 0: yearsChange -= 10; break;
                    case 1: monthsChange -= 3; break;
                    case 2: daysChange -= 10; break;
                    case 3: hoursChange -= 6; break;
                    case 4: minutesChange -= 10; break;
                }
            } else if (kDown & (HidNpadButton_R | HidNpadButton_ZR)) {
                switch (arrow) {
                    case 0: yearsChange += 10; break;
                    case 1: monthsChange += 3; break;
                    case 2: daysChange += 10; break;
                    case 3: hoursChange += 6; break;
                    case 4: minutesChange += 10; break;
                }
            }

             if (kDown & HidNpadButton_Left) {
                arrow = (arrow - 1 + 5) % 5;
            } else if (kDown & HidNpadButton_Right) {
                arrow = (arrow + 1) % 5;
            }

            char timeToSetStr[20];
            strftime(timeToSetStr, sizeof(timeToSetStr), "%Y/%m/%d %H:%M:%S", p_tm_timeToSet);
            printf(CONSOLE_ESC(1A)"\r%s\n", timeToSetStr);
            char arrowStr[sizeof(timeToSetStr)];
            memset(arrowStr, ' ', sizeof(arrowStr) - 1);
            memset(arrowStr + arrow_pos[arrow], '^', arrow_len[arrow]);
            arrowStr[sizeof(arrowStr) - 1] = '\0';
            printf("%s", arrowStr);

            consoleUpdate(NULL);
        }
    }
}
