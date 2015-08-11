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

const char * getFreeplayDipGroup()
{
	switch (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) {
	case HARDWARE_SNK_NEOGEO | HARDWARE_PREFIX_CARTRIDGE:
	case HARDWARE_CAPCOM_CPS1:
	case HARDWARE_CAPCOM_CPS1_GENERIC:
	case HARDWARE_CAPCOM_CPS1_QSOUND:
		return "free play";
	case HARDWARE_PACMAN:
		return "coinage";
	case HARDWARE_PREFIX_KONAMI:
	case HARDWARE_KONAMI_68K_Z80:
		return "coin a";
	}

	return NULL;
}

const char * getFreeplayDipSetting()
{
	switch (BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) {
	case HARDWARE_SNK_NEOGEO | HARDWARE_PREFIX_CARTRIDGE:
	case HARDWARE_CAPCOM_CPS1:
	case HARDWARE_CAPCOM_CPS1_GENERIC:
	case HARDWARE_CAPCOM_CPS1_QSOUND:
		return "on";
	case HARDWARE_PACMAN:
	case HARDWARE_PREFIX_KONAMI:
	case HARDWARE_KONAMI_68K_Z80:
		return "free play";
	}

	return NULL;
}

int enableFreeplay()
{
	const char *dipGroup = getFreeplayDipGroup();
	const char *dipSetting = getFreeplayDipSetting();

	BurnDIPInfo bdi;
	if (dipGroup == NULL || dipSetting == NULL) {
		fprintf(stderr, "Don't know how to enable freeplay.\n");
		for (int i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++) {
			char flags[9], mask[9], setting[9];
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

		return 0;
	}

	int dipOffset = 0;
	for (int i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++) {
		if (bdi.nFlags == 0xF0) {
			dipOffset = bdi.nInput;
			break;
		}
	}

	for (int i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++) {
		if ((bdi.nFlags & 0x40) && bdi.szText) {
			if (strcasecmp(bdi.szText, dipGroup) == 0) {
				while (BurnDrvGetDIPInfo(&bdi, ++i) == 0 && !(bdi.nFlags & 0x40)) {
					if (strcasecmp(bdi.szText, dipSetting) == 0) {
						struct GameInp *pgi = GameInp + bdi.nInput + dipOffset;
						pgi->Input.Constant.nConst = (pgi->Input.Constant.nConst & ~bdi.nMask) | (bdi.nSetting & bdi.nMask);
						printf("Freeplay enabled\n");
						break;
					}
				}

				break;
			}
		}
	}

	return 1;
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
		fprintf(stderr, "%s is not supported by FB Alpha\n", argv[1]);
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
		enableFreeplay();
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
