#include "godot_nx.h"
#include "os_nx.h"

int main(int argc, char *argv[]) {
	socketInitializeDefault();
	nxlinkStdio();

	romfsInit();

    OS_NX os;
    //TODO

    romfsExit();
	socketExit();

    return os.get_exit_code();
}
