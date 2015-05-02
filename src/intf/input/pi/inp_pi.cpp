// Module for input using SDL
#include <SDL/SDL.h>
#include <unistd.h>

#include "burner.h"
#include "inp_sdl_keys.h"

extern "C" {
#include "inp_udev.h"
#include "cJSON.h"
}

// Note that these constants are tied to the JOY_ macros!
#define MAX_JOYSTICKS   8
#define MAX_JOY_BUTTONS 28

static unsigned int joyButtonStates[MAX_JOYSTICKS];
static int *joyLookupTable = NULL;
static int keyLookupTable[512];

static int mouseScanned = 0;

static void scanKeyboard();
static void scanJoysticks();
static void scanMouse();

static void resetJoystickMap();
static bool usesStreetFighterLayout();
static int checkMouseState(unsigned int nSubCode);

#define JOY_DIR_LEFT  0x00
#define JOY_DIR_RIGHT 0x01
#define JOY_DIR_UP    0x02
#define JOY_DIR_DOWN  0x03

#define JOY_MAP_DIR(joy,dir) ((((joy)&0xff)<<8)|((dir)&0x3))
#define JOY_MAP_BUTTON(joy,button) ((((joy)&0xff)<<8)|(((button)+4)&0x1f))
#define JOY_IS_DOWN(value) (joyButtonStates[((value)>>8)&0x7]&(1<<((value)&0xff)))
#define JOY_CLEAR(value) joyButtonStates[((value)>>8)&0x7]&=~(1<<((value)&0xff))
#define KEY_IS_DOWN(key) keyState[(key)]

#define JOY_DEADZONE 0x4000

static unsigned char* keyState;
static struct {
	unsigned char buttons;
	int xdelta;
	int ydelta;
} mouseState;


static char* globFile(const char *path);

///

extern int nExitEmulator;

static int nInitedSubsytems = 0;
static SDL_Joystick* JoyList[MAX_JOYSTICKS];
static int nJoystickCount = 0;						// Number of joysticks connected to this machine

static int piInputExit()
{
	// Close all joysticks
	for (int i = 0; i < MAX_JOYSTICKS; i++) {
		if (JoyList[i]) {
			SDL_JoystickClose(JoyList[i]);
			JoyList[i] = NULL;
		}
	}

	free(joyLookupTable);
	joyLookupTable = NULL;

	nJoystickCount = 0;

	if (!(nInitedSubsytems & SDL_INIT_JOYSTICK)) {
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}

	nInitedSubsytems = 0;
	udevShutdown();

	return 0;
}

static int piInputInit()
{
	piInputExit();

	// Allocate memory for the lookup table
	if ((joyLookupTable = (int *)malloc(0x8000 * sizeof(int))) == NULL) {
		return 1;
	}

	memset(&JoyList, 0, sizeof(JoyList));

	nInitedSubsytems = SDL_WasInit(SDL_INIT_JOYSTICK);

	udevInit();
	if (!(nInitedSubsytems & SDL_INIT_JOYSTICK)) {
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	}

	// Set up the joysticks
	nJoystickCount = SDL_NumJoysticks();
	for (int i = 0; i < nJoystickCount; i++) {
		JoyList[i] = SDL_JoystickOpen(i);
	}
	SDL_JoystickEventState(SDL_IGNORE);

	resetJoystickMap();

	return 0;
}

static int piInputStart()
{
	// Update SDL event queue
	SDL_PumpEvents();

	scanJoysticks();
	scanKeyboard();

	// Mouse not read this frame
	mouseScanned = 0;

	return 0;
}

static int piInputState(int nCode)
{
	if (nCode < 0) {
		return 0;
	}

	if (nCode < 0x100) {
		int mapped = keyLookupTable[nCode];
		if (KEY_IS_DOWN(mapped)) {
			return 1;
		}
	}

	if (/* nCode >= 0x4000 && */ nCode < 0x8000) {
		int mapped = joyLookupTable[nCode];
		if (mapped != -1) {
			if (JOY_IS_DOWN(mapped)) {
				JOY_CLEAR(mapped);
				return 1;
			}
		}
	}

	if (nCode >= 0x8000 && nCode < 0xC000) {
		// Codes 8000-C000 = Mouse
		if ((nCode - 0x8000) >> 8) {
			// Only the system mouse is supported by SDL
			return 0;
		}
		if (!mouseScanned) {
			scanMouse();
			mouseScanned = 1;
		}
		return checkMouseState(nCode & 0xFF);
	}

	return 0;
}

// Check a subcode (the 80xx bit in 8001, 8102 etc) for a mouse input code
static int checkMouseState(unsigned int nSubCode)
{
	switch (nSubCode & 0x7F) {
	case 0: return (mouseState.buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	case 1: return (mouseState.buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	case 2: return (mouseState.buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	}

	return 0;
}

static void scanMouse()
{
	mouseState.buttons = SDL_GetRelativeMouseState(&(mouseState.xdelta),
		&(mouseState.ydelta));
}

static void scanKeyboard()
{
	static Uint8 emptyStates[512] = { 0 };
	if ((keyState = SDL_GetKeyState(NULL)) == NULL) {
		keyState = emptyStates;
	}

	// FIXME
	if (keyState[SDLK_F12]) {
		nExitEmulator = 1;
	}
}

static void scanJoysticks()
{
	SDL_JoystickUpdate();
	
	int joyCount = nJoystickCount;
	if (joyCount > MAX_JOYSTICKS) {
		joyCount = MAX_JOYSTICKS;
	}
	for (int joy = 0; joy < joyCount; joy++) {
		joyButtonStates[joy] = 0;
		SDL_Joystick *joystick = JoyList[joy];

		int xPos = SDL_JoystickGetAxis(joystick, 0);
		int yPos = SDL_JoystickGetAxis(joystick, 1);

		joyButtonStates[joy] |= ((xPos < -JOY_DEADZONE) << JOY_DIR_LEFT);
		joyButtonStates[joy] |= ((xPos > JOY_DEADZONE) << JOY_DIR_RIGHT);
		joyButtonStates[joy] |= ((yPos < -JOY_DEADZONE) << JOY_DIR_UP);
		joyButtonStates[joy] |= ((yPos > JOY_DEADZONE) << JOY_DIR_DOWN);

		int buttonCount = SDL_JoystickNumButtons(joystick);
		if (buttonCount > MAX_JOY_BUTTONS) {
			buttonCount = MAX_JOY_BUTTONS;
		}
		for (int button = 0, shift = 4; button < buttonCount; button++, shift++) {
			int state = SDL_JoystickGetButton(joystick, button);
			joyButtonStates[joy] |= ((state & 1) << shift);
		}
	}
}

static bool usesStreetFighterLayout()
{
	int fireButtons = 0;
	struct BurnInputInfo bii;

	for (UINT32 i = 0; i < 0x1000; i++) {
		bii.szName = NULL;
		if (BurnDrvGetInputInfo(&bii,i)) {
			break;
		}
		
		BurnDrvGetInputInfo(&bii, i);
		if (bii.szName == NULL) {
			bii.szName = "";
		}

		bool bPlayerInInfo = (toupper(bii.szInfo[0]) == 'P' && bii.szInfo[1] >= '1' && bii.szInfo[1] <= '4'); // Because some of the older drivers don't use the standard input naming.
		bool bPlayerInName = (bii.szName[0] == 'P' && bii.szName[1] >= '1' && bii.szName[1] <= '4');

		if (bPlayerInInfo || bPlayerInName) {
			INT32 nPlayer = 0;

			if (bPlayerInName) {
				nPlayer = bii.szName[1] - '1';
			}
			if (bPlayerInInfo && nPlayer == 0) {
				nPlayer = bii.szInfo[1] - '1';
			}
			if (nPlayer == 0) {
				if (strncmp(" fire", bii.szInfo + 2, 5) == 0) {
					fireButtons++;
				}
			}
		}
	}

	int hardwareMask = BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK;
	return fireButtons >= 6 &&
		(hardwareMask == HARDWARE_CAPCOM_CPS1 ||
		hardwareMask == HARDWARE_CAPCOM_CPS2 ||
		hardwareMask == HARDWARE_CAPCOM_CPS3);
}

struct JoyConfig {
	char **labels;
	int labelCount;
};

static struct JoyConfig* createJoyConfig(cJSON *root)
{
	struct JoyConfig *jc = (struct JoyConfig *)malloc(sizeof(struct JoyConfig));
	if (jc) {
		jc->labels = NULL;
		jc->labelCount = 0;

		cJSON *labelParent = cJSON_GetObjectItem(root, "buttonLabels");
		cJSON *labelNode;
		
		if (labelParent) {
			labelNode = labelParent->child;
			while (labelNode) {
				if (strncmp(labelNode->string, "button", 6) == 0) {
					int index = atoi(labelNode->string + 6);
					if (index > 0 && index <= MAX_JOY_BUTTONS && index > jc->labelCount) {
						jc->labelCount = index;
					}
				}
				labelNode = labelNode->next;
			}
			
			if (jc->labelCount > 0) {
				jc->labels = (char **)calloc(jc->labelCount, sizeof(char *));
				if (jc->labels) {
					labelNode = labelParent->child;
					while (labelNode) {
						int index = atoi(labelNode->string + 6) - 1;
						if (index >= 0 && index < MAX_JOY_BUTTONS) {
							jc->labels[index] = strdup(labelNode->valuestring);
						}
						labelNode = labelNode->next;
					}
				}
			}
		}
	}
	return jc;
}

static void dumpJoyConfig(struct JoyConfig *jc)
{
	for (int i = 0; i < jc->labelCount; i++) {
		fprintf(stderr, "labels[%d]='%s'\n", i, jc->labels[i]);
	}
}

static void destroyJoyConfig(struct JoyConfig *jc)
{
	for (int i = 0; i < jc->labelCount; i++) {
		free(jc->labels[i]);
	}
	free(jc->labels);
	jc->labels = NULL;
	jc->labelCount = 0;
	free(jc);
}

static int findButton(struct JoyConfig *jc, const char *label)
{
	for (int i = 0; i < jc->labelCount; i++) {
		if (strcasecmp(jc->labels[i], label) == 0) {
			return i;
		}
	}

	if (strncmp(label, "button", 6) == 0) {
		int index = atoi(label + 6) - 1;
		if (index >= 0 && index < MAX_JOY_BUTTONS) {
			return index;
		}
	}

	return -1;
}

static void parseButtons(int player, struct JoyConfig *jc, cJSON *json)
{
	cJSON *node = json->child;
	while (node) {
		int index = findButton(jc, node->string);
		if (index > -1) {
			int code;
			if ((code = InputFindCode(node->valuestring)) != -1) {
				joyLookupTable[code] = JOY_MAP_BUTTON(player, index);
			}
		}
		node = node->next;
	}
}

static void parsePlayerConfig(int player, struct JoyConfig *jc, cJSON *json)
{
	cJSON *node;
	int code;

	const char *dirnames[] = { "up","down","left","right" };
	int dirconsts[] = { JOY_DIR_UP,JOY_DIR_DOWN,JOY_DIR_LEFT,JOY_DIR_RIGHT };
	
	for (int i = 0; i < 4; i++) {
		if ((node = cJSON_GetObjectItem(json, dirnames[i]))) {
			if ((code = InputFindCode(node->valuestring)) != -1) {
				joyLookupTable[code] = JOY_MAP_DIR(player, dirconsts[i]);
			}
		}
	}
	
	parseButtons(player, jc, json);
	if (usesStreetFighterLayout()) {
		cJSON *sf = cJSON_GetObjectItem(json, "sfLayout");
		if (sf) {
			parseButtons(player, jc, sf);
		}
	}
}

static void readConfigFile(const char *path)
{
	char *contents = globFile(path);
	if (contents) {
		cJSON *root = cJSON_Parse(contents);
		if (root) {
			struct JoyConfig *jc = createJoyConfig(root);
			
			cJSON *player;
			if ((player = cJSON_GetObjectItem(root, "p1"))) {
				parsePlayerConfig(0, jc, player);
			}
			if ((player = cJSON_GetObjectItem(root, "p2"))) {
				parsePlayerConfig(1, jc, player);
			}
			if ((player = cJSON_GetObjectItem(root, "p3"))) {
				parsePlayerConfig(2, jc, player);
			}
			if ((player = cJSON_GetObjectItem(root, "p4"))) {
				parsePlayerConfig(3, jc, player);
			}

			destroyJoyConfig(jc);
			cJSON_Delete(root);
		}
		free(contents);
	}
}

static void resetJoystickMap()
{
	for (int i = 0; i < 0x8000; i++) {
		joyLookupTable[i] = -1;
	}
	
	for (int i = 0; i < 512; i++) {
		int code = SDLtoFBK[i];
		keyLookupTable[code] = (code > 0) ? i : -1;
	}

	if (udevJoystickCount() > 0) {
		const char *devId = udevDeviceId(0);
		if (devId != NULL) {
			char path[100];
			snprintf(path, 99, "%s.joy", devId);
			char *colon = strchr(path, ':');
			if (colon) {
				*colon = '-';
			}
			if (access(path, F_OK) != -1) {
				fprintf(stderr, "detected device \"%s\" - found a joy config file \"%s\"\n",
					udevDeviceName(0), path);

				readConfigFile(path);
			}
		}
	}
}

static char* globFile(const char *path)
{
	char *contents = NULL;
	
	FILE *file = fopen(path,"r");
	if (file) {
		// Determine size
		fseek(file, 0L, SEEK_END);
		long size = ftell(file);
		rewind(file);
	
		// Allocate memory
		contents = (char *)malloc(size + 1);
		if (contents) {
			contents[size] = '\0';
			// Read contents
			if (fread(contents, size, 1, file) != 1) {
				free(contents);
				contents = NULL;
			}
		}

		fclose(file);
	}
	
	return contents;
}

struct InputInOut InputInOutPi = { piInputInit, piInputExit, NULL, piInputStart, piInputState, NULL, NULL, NULL, NULL, NULL, _T("Raspberry Pi input") };
