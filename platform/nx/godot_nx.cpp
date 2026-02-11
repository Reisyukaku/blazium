#include "godot_nx.h"
#include "os_nx.h"

void CheckIfAppletMode() {
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
    os.set_executable_path(argv[0]);

    char *cwd = (char *)malloc(PATH_MAX);
	getcwd(cwd, PATH_MAX);

    Error err = Main::setup(argv[0], argc - 1, &argv[1]);
	if (err == OK && Main::start() == EXIT_SUCCESS) 
	    os.run();
    
	Main::cleanup();

	chdir(cwd);
	free(cwd);

    romfsExit();
	socketExit();

    return os.get_exit_code();
}
