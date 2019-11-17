#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>
#include <proc_ui/procui.h>
#include <coreinit/foreground.h>

#include <coreinit/screen.h>
#include <sysapp/launch.h>
#include <coreinit/dynload.h>
#include <nn/ac.h>
#include <stdarg.h>
#include <whb/proc.h>
#include <stdlib.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <stdio.h>
#include <whb/libmanager.h>
#include "utils/logger.h"
#include "utils/utils.h"
#include "ftp.h"
#include "virtualpath.h"
#include "net.h"

#define PORT                    21
#define MAX_CONSOLE_LINES_TV    27
#define MAX_CONSOLE_LINES_DRC   18

static char * consoleArrayTv[MAX_CONSOLE_LINES_TV];
static char * consoleArrayDrc[MAX_CONSOLE_LINES_DRC];

extern "C" void console_printf(const char *format, ...) {


    return;
    char * tmp = NULL;

    va_list va;
    va_start(va, format);
    if((vasprintf(&tmp, format, va) >= 0) && tmp) {
        if(consoleArrayTv[0])
            free(consoleArrayTv[0]);
        if(consoleArrayDrc[0])
            free(consoleArrayDrc[0]);

        for(int i = 1; i < MAX_CONSOLE_LINES_TV; i++)
            consoleArrayTv[i-1] = consoleArrayTv[i];

        for(int i = 1; i < MAX_CONSOLE_LINES_DRC; i++)
            consoleArrayDrc[i-1] = consoleArrayDrc[i];

        if(strlen(tmp) > 79)
            tmp[79] = 0;

        consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = (char*)malloc(strlen(tmp) + 1);
        if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
            strcpy(consoleArrayTv[MAX_CONSOLE_LINES_TV-1], tmp);

        consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = (tmp);
    }
    va_end(va);

    // Clear screens
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);


    for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++) {
        if(consoleArrayTv[i])
            OSScreenPutFontEx(SCREEN_TV, 0, i, consoleArrayTv[i]);
    }

    for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++) {
        if(consoleArrayDrc[i])
            OSScreenPutFontEx(SCREEN_DRC, 0, i, consoleArrayDrc[i]);
    }

    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

//just to be able to call async
void someFunc(void *arg) {
    (void)arg;
}


static uint32_t
procSaveCallback(void *context) {
    OSSavesDone_ReadyToRelease();
    return 0;
}

uint32_t hostIpAddress = 0;

/* Entry point */
extern "C" int Menu_Main(void) {
    WHBInitializeSocketLibrary();

    nn::ac::ConfigIdNum configId;

    nn::ac::Initialize();
    nn::ac::GetStartupId(&configId);
    nn::ac::Connect(configId);

    ACGetAssignedAddress(&hostIpAddress);


    ProcUIInitEx(&procSaveCallback, NULL);

    log_init();
    DEBUG_FUNCTION_LINE("Starting launcher\n");

    DEBUG_FUNCTION_LINE("Function exports loaded\n");

    //!*******************************************************************
    //!                    Initialize heap memory                        *
    //!*******************************************************************
    DEBUG_FUNCTION_LINE("Initialize memory management\n");
    //! We don't need bucket and MEM1 memory so no need to initialize
    //memoryInitialize();

    //!*******************************************************************
    //!                        Initialize FS                             *
    //!*******************************************************************
    DEBUG_FUNCTION_LINE("Mount SD partition\n");

    int fsaFd = -1;
    int iosuhaxMount = 0;

    int res = IOSUHAX_Open(NULL);
    if(res < 0) {
        DEBUG_FUNCTION_LINE("IOSUHAX_open failed\n");
        VirtualMountDevice("fs:/");
    } else {
        iosuhaxMount = 1;
        //fatInitDefault();

        fsaFd = IOSUHAX_FSA_Open();
        if(fsaFd < 0) {
            DEBUG_FUNCTION_LINE("IOSUHAX_FSA_Open failed\n");
        }

        mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
        mount_fs("storage_odd_tickets", fsaFd, "/dev/odd01", "/vol/storage_odd_tickets");
        mount_fs("storage_odd_updates", fsaFd, "/dev/odd02", "/vol/storage_odd_updates");
        mount_fs("storage_odd_content", fsaFd, "/dev/odd03", "/vol/storage_odd_content");
        mount_fs("storage_odd_content2", fsaFd, "/dev/odd04", "/vol/storage_odd_content2");
        mount_fs("storage_slc", fsaFd, NULL, "/vol/system");
        mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
        mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");

        VirtualMountDevice("fs:/");
        VirtualMountDevice("slccmpt01:/");
        VirtualMountDevice("storage_odd_tickets:/");
        VirtualMountDevice("storage_odd_updates:/");
        VirtualMountDevice("storage_odd_content:/");
        VirtualMountDevice("storage_odd_content2:/");
        VirtualMountDevice("storage_slc:/");
        VirtualMountDevice("storage_mlc:/");
        VirtualMountDevice("storage_usb:/");
        VirtualMountDevice("usb:/");
    }

    for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
        consoleArrayTv[i] = NULL;

    for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
        consoleArrayDrc[i] = NULL;

    // Prepare screen
    int screen_buf0_size = 0;

    // Init screen and screen buffers
    /*OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(SCREEN_TV);
    OSScreenSetBufferEx(SCREEN_TV, (void *)0xF4000000);
    OSScreenSetBufferEx(SCREEN_DRC, (void *)(0xF4000000 + screen_buf0_size));

    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);

    // Clear screens
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);

    // Flip buffers
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);*/

    console_printf("FTPiiU v0.4u2 is listening on %u.%u.%u.%u:%i", (network_gethostip() >> 24) & 0xFF, (network_gethostip() >> 16) & 0xFF, (network_gethostip() >> 8) & 0xFF, (network_gethostip() >> 0) & 0xFF, PORT);

    DEBUG_FUNCTION_LINE("starting server\n");

    int serverSocket = create_server(PORT);
    DEBUG_FUNCTION_LINE("Server socket = %d\n",serverSocket);
    int network_down = 0;

    DEBUG_FUNCTION_LINE("going into procui loop\n");

    bool requestedExit = false;

    while(true) {
        bool doExit = false;
        switch(ProcUIProcessMessages(true)) {
        case PROCUI_STATUS_EXITING: {
            DEBUG_FUNCTION_LINE("Exiting\n");
            doExit = true;
            break;
        }
        case PROCUI_STATUS_RELEASE_FOREGROUND: {
            DEBUG_FUNCTION_LINE("PROCUI_STATUS_RELEASE_FOREGROUND\n");
            if(serverSocket >= 0) {
                DEBUG_FUNCTION_LINE("Clean up stuff\n");
                cleanup_ftp();
                network_close(serverSocket);
                serverSocket = -1;
            }
            ProcUIDrawDoneRelease();
            break;
        }
        case PROCUI_STATUS_IN_FOREGROUND: {
            if(serverSocket < 0) {
                DEBUG_FUNCTION_LINE("back in PROCUI_STATUS_IN_FOREGROUND, createig network\n");
                serverSocket = create_server(PORT);

            }
            if(serverSocket >= 0) {
                //DEBUG_FUNCTION_LINE("Server socket = %d\n",serverSocket);
                network_down = process_ftp_events(serverSocket);
                if(network_down && !requestedExit) {


                    // request exit once.
                    cleanup_ftp();
                    network_close(serverSocket);
                    serverSocket = -1;
                    DEBUG_FUNCTION_LINE("Network is down, exit to sys menu\n");
                    SYSRelaunchTitle(0, NULL);
                    requestedExit =true;

                }
            }
            break;
        }
        case PROCUI_STATUS_IN_BACKGROUND:
        default:
            break;
        }
        OSSleepTicks(OSMillisecondsToTicks(16));

        if(doExit) {
            break;
        }
    }

    DEBUG_FUNCTION_LINE("loop done\n");

    //! free memory
    for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++) {
        if(consoleArrayTv[i])
            free(consoleArrayTv[i]);
    }

    for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++) {
        if(consoleArrayDrc[i])
            free(consoleArrayDrc[i]);
    }



    //!*******************************************************************
    //!                    Enter main application                        *
    //!*******************************************************************

    DEBUG_FUNCTION_LINE("Unmount SD\n");

    if(iosuhaxMount) {
        IOSUHAX_sdio_disc_interface.shutdown();
        IOSUHAX_usb_disc_interface.shutdown();

        unmount_fs("slccmpt01");
        unmount_fs("storage_odd_tickets");
        unmount_fs("storage_odd_updates");
        unmount_fs("storage_odd_content");
        unmount_fs("storage_odd_content2");
        unmount_fs("storage_slc");
        unmount_fs("storage_mlc");
        unmount_fs("storage_usb");
        IOSUHAX_FSA_Close(fsaFd);
        IOSUHAX_Close();
    }

    UnmountVirtualPaths();
	
	while(WHBProcIsRunning()) {
		
	}

    WHBProcShutdown();

    nn::ac::Finalize();

    //OSScreenShutdown();

    DEBUG_FUNCTION_LINE("Release memory\n");

    return EXIT_SUCCESS;
}

