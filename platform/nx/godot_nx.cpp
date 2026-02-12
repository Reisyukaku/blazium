#include "godot_nx.h"
#include "main/main.h"
#include "os_nx.h"

int CheckIfAppletMode() {
    int apptype = appletGetAppletType();
	if (apptype != AppletType_Application && apptype != AppletType_SystemApplication) {
        //TODO
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	socketInitializeDefault();
	nxlinkStdio();

	romfsInit();

    int ret = CheckIfAppletMode();
    if(ret != EXIT_SUCCESS) 
        return ret;

    OS_NX os;

    char *cwd = (char *)malloc(PATH_MAX);
	getcwd(cwd, PATH_MAX);

    Error err = Main::setup(argv[0], argc - 1, &argv[1]);
	if (err == OK && Main::start() == EXIT_SUCCESS) {
        MainLoop *loop = os.get_main_loop();
        if(!loop) return EXIT_FAILURE;

        loop->initialize();
        while (appletMainLoop()) {
            if (Main::iteration())
			    break;
        }
        loop->finalize();
    }
    
	Main::cleanup(true);

	chdir(cwd);
	free(cwd);

    romfsExit();
	socketExit();

    return os.get_exit_code();
}
