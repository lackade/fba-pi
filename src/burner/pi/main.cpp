#include "burner.h"
#include "config.h"
#include <sys/stat.h>
#include <unistd.h>

int nAppVirtualFps = 6000;			// App fps * 100
bool bRunPause = 0;
bool bAlwaysProcessKeyboardInput=0;

int main(int argc, char *argv[])
{
	piLoadConfig();

	SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO);

	BurnLibInit();

	if (argc < 2) {
		fprintf(stdout, "Usage: %s <romname>\n", argv[0]);
		fprintf(stdout, "e.g.: %s mslug\n", argv[0]);

		return 0;
	}

	int driverId = -1;
	if (argc == 2) {
		for (int i = 0; i < nBurnDrvCount; i++) {
			nBurnDrvActive = i;
			if (strcmp(BurnDrvGetTextA(0), argv[1]) == 0) {
				driverId = i;
				break;
			}
		}
	}

	if (driverId < 0) {
		fprintf(stderr, "%s is not supported by FB Alpha\n", argv[1]);
		return 1;
	}

	// Create the nvram directory, if it doesn't exist
	const char *nvramPath = "./nvram";
	if (access(nvramPath, F_OK) == -1) {
		fprintf(stderr, "Creating NVRAM directory at \"%s\"\n", nvramPath);
        mkdir(nvramPath, 0777);
	}

	bBurnUseASMCPUEmulation = 0;
	bCheatsAllowed = false;

	DrvInit(driverId, 0);

	RunMessageLoop();

	DrvExit();
	MediaExit();

	piSaveConfig();
	BurnLibExit();
	SDL_Quit();

	return 0;
}


/* const */ TCHAR* ANSIToTCHAR(const char* pszInString, TCHAR* pszOutString, int nOutSize)
{
	if (pszOutString) {
		_tcscpy(pszOutString, pszInString);
		return pszOutString;
	}

	return (TCHAR*)pszInString;
}


/* const */ char* TCHARToANSI(const TCHAR* pszInString, char* pszOutString, int nOutSize)
{
	if (pszOutString) {
		strcpy(pszOutString, pszInString);
		return pszOutString;
	}

	return (char*)pszInString;
}


bool AppProcessKeyboardInput()
{
	return true;
}
