#include "burner.h"
#include "config.h"
#include <sys/stat.h>
#include <unistd.h>

extern int nKioskTimeout;

int nAppVirtualFps = 6000;			// App fps * 100
bool bRunPause = 0;
bool bAlwaysProcessKeyboardInput=0;

void formatBinary(int number, int sizeBytes, char *dest, int len)
{
	int size = sizeBytes * 8;
	if (size + 1 > len) {
		*dest = '\0';
		return;
	}

	char *ch = dest + size;
	*ch = '\0';
	ch--;

	for (int i = 0; i < size; i++, ch--) {
		*ch = (number & 1) + '0';
		number >>= 1;
	}
}

void dumpDipSwitches()
{
	BurnDIPInfo bdi;
	char flags[9], mask[9], setting[9];

	for (int i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++) {
		formatBinary(bdi.nFlags, sizeof(bdi.nFlags), flags, sizeof(flags));
		formatBinary(bdi.nMask, sizeof(bdi.nMask), mask, sizeof(mask));
		formatBinary(bdi.nSetting, sizeof(bdi.nSetting), setting, sizeof(setting));

		printf("-- % 3d - 0x%02x (%s) - 0x%02x (%s) - 0x%02x (%s) - %s\n",
			bdi.nInput,
			bdi.nFlags, flags,
			bdi.nMask, mask,
			bdi.nSetting, setting,
			bdi.szText);
	}
}

int enableFreeplay()
{
	int dipOffset = 0;
	int switchFound = 0;
	BurnDIPInfo dipSwitch;

	for (int i = 0; !switchFound && BurnDrvGetDIPInfo(&dipSwitch, i) == 0; i++) {
		if (dipSwitch.nFlags == 0xF0) {
			dipOffset = dipSwitch.nInput;
		}
		if (dipSwitch.szText && (strcasecmp(dipSwitch.szText, "freeplay") == 0
			|| strcasecmp(dipSwitch.szText, "free play") == 0)) {
			// Found freeplay. Is it a group or the actual switch?
			if (dipSwitch.nFlags & 0x40) {
				// It's a group. Find the switch
				for (int j = i + 1; !switchFound && BurnDrvGetDIPInfo(&dipSwitch, j) == 0 && !(dipSwitch.nFlags & 0x40); j++) {
					if (dipSwitch.szText && strcasecmp(dipSwitch.szText, "on") == 0) {
						// Found the switch
						switchFound = 1;
					}
				}
			} else {
				// It's a switch
				switchFound = 1;
			}
		}
	}

	if (switchFound) {
		struct GameInp *pgi = GameInp + dipSwitch.nInput + dipOffset;
		pgi->Input.Constant.nConst = (pgi->Input.Constant.nConst & ~dipSwitch.nMask) | (dipSwitch.nSetting & dipSwitch.nMask);
	}

	return switchFound;
}

int main(int argc, char *argv[])
{
	int freeplay = 0;
	const char *romname = NULL;

	for (int i = 1; i < argc; i++) {
		if (*argv[i] == '-') {
			if (strcasecmp(argv[i] + 1, "f") == 0) {
				freeplay = 1;
			} else if (strcasecmp(argv[i] + 1, "k") == 0) {
				if (++i < argc) {
					int secs = atoi(argv[i]);
					if (secs > 0) {
						nKioskTimeout = secs;
						printf("Kiosk mode enabled (%d seconds)\n", secs);
					}
				}
			}
		} else {
			romname = argv[i];
		}
	}

	if (romname == NULL) {
		printf("Usage: %s [-f] [-k seconds] <romname>\n", argv[0]);
		printf("e.g.: %s mslug\n", argv[0]);

		return 0;
	}

	piLoadConfig();
	SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO);
	BurnLibInit();

	int driverId = -1;
	for (int i = 0; i < nBurnDrvCount; i++) {
		nBurnDrvActive = i;
		if (strcmp(BurnDrvGetTextA(0), romname) == 0) {
			driverId = i;
			break;
		}
	}

	if (driverId < 0) {
		fprintf(stderr, "%s is not supported by FB Alpha\n", romname);
		return 1;
	}

	printf("Starting %s\n", romname);

	// Create the nvram directory, if it doesn't exist
	const char *nvramPath = "./nvram";
	if (access(nvramPath, F_OK) == -1) {
		fprintf(stderr, "Creating NVRAM directory at \"%s\"\n", nvramPath);
        mkdir(nvramPath, 0777);
	}

	bBurnUseASMCPUEmulation = 0;
	bCheatsAllowed = false;

	DrvInit(driverId, 0);

	if (freeplay) {
		if (enableFreeplay()) {
			printf("Freeplay enabled\n");
		} else {
			fprintf(stderr, "Don't know how to enable freeplay\n");
			dumpDipSwitches();
		}
	}

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
