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
#include "console.h"

#define PORT                    21

static uint32_t
procSaveCallback(void *context) {
    OSSavesDone_ReadyToRelease();
    return 0;
}

/* Entry point */
extern "C" int Menu_Main(void) {
    int fsaFd = -1, iosuhaxMount = 0;

    ProcUIInitEx(&procSaveCallback, NULL);

    net_init();
    log_init();
    console_init();

    DEBUG_FUNCTION_LINE("add SD partition\n");
    AddVirtualPath("sd", "/sd", "fs:/vol/external01/");

    DEBUG_FUNCTION_LINE("mount IOSUHAX devices\n");
    int res = IOSUHAX_Open(NULL);
    if(res >= 0) {
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

    console_printf("FTPiiU v0.4u2 is listening on %u.%u.%u.%u:%i", (network_gethostip() >> 24) & 0xFF, (network_gethostip() >> 16) & 0xFF, (network_gethostip() >> 8) & 0xFF, (network_gethostip() >> 0) & 0xFF, PORT);

    DEBUG_FUNCTION_LINE("starting server\n");

    int serverSocket = create_server(PORT);
    DEBUG_FUNCTION_LINE("server socket = %d\n",serverSocket);
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
                console_printf("Stopped FTP server\n");
            }
            console_release_foreground();
            ProcUIDrawDoneRelease();
            break;
        }
        case PROCUI_STATUS_IN_FOREGROUND: {
            console_in_foreground();
            if(serverSocket < 0) {
                DEBUG_FUNCTION_LINE("back in PROCUI_STATUS_IN_FOREGROUND, creating network\n");
                serverSocket = create_server(PORT);
                console_printf("Started FTP server\n");
            }
            if(serverSocket >= 0) {
                //DEBUG_FUNCTION_LINE("server socket = %d\n",serverSocket);
                network_down = process_ftp_events(serverSocket);
                if(network_down && !requestedExit) {


                    // request exit once.
                    cleanup_ftp();
                    network_close(serverSocket);
                    serverSocket = -1;
                    DEBUG_FUNCTION_LINE("Network is down, exit to sys menu\n");
                    SYSRelaunchTitle(0, NULL);
                    requestedExit = true;

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


    DEBUG_FUNCTION_LINE("unmounting IOSUHAX devices\n");
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

    console_deinit();
    net_deinit();

    while(WHBProcIsRunning());
    WHBProcShutdown();

    return EXIT_SUCCESS;
}

