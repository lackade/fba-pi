#include <cstddef>
#include "string.h"

static const char *FBK_ALLSTRINGS[] = {
	"",
	"FBK_ESCAPE", // 0x01
	"FBK_1", // 0x02
	"FBK_2", // 0x03
	"FBK_3", // 0x04
	"FBK_4", // 0x05
	"FBK_5", // 0x06
	"FBK_6", // 0x07
	"FBK_7", // 0x08
	"FBK_8", // 0x09
	"FBK_9", // 0x0A
	"FBK_0", // 0x0B
	"FBK_MINUS", // 0x0C
	"FBK_EQUALS", // 0x0D
	"FBK_BACK", // 0x0E
	"FBK_TAB", // 0x0F
	"FBK_Q", // 0x10
	"FBK_W", // 0x11
	"FBK_E", // 0x12
	"FBK_R", // 0x13
	"FBK_T", // 0x14
	"FBK_Y", // 0x15
	"FBK_U", // 0x16
	"FBK_I", // 0x17
	"FBK_O", // 0x18
	"FBK_P", // 0x19
	"FBK_LBRACKET", // 0x1A
	"FBK_RBRACKET", // 0x1B
	"FBK_RETURN", // 0x1C
	"FBK_LCONTROL", // 0x1D
	"FBK_A", // 0x1E
	"FBK_S", // 0x1F
	"FBK_D", // 0x20
	"FBK_F", // 0x21
	"FBK_G", // 0x22
	"FBK_H", // 0x23
	"FBK_J", // 0x24
	"FBK_K", // 0x25
	"FBK_L", // 0x26
	"FBK_SEMICOLON", // 0x27
	"FBK_APOSTROPHE", // 0x28
	"FBK_GRAVE", // 0x29
	"FBK_LSHIFT", // 0x2A
	"FBK_BACKSLASH", // 0x2B
	"FBK_Z", // 0x2C
	"FBK_X", // 0x2D
	"FBK_C", // 0x2E
	"FBK_V", // 0x2F
	"FBK_B", // 0x30
	"FBK_N", // 0x31
	"FBK_M", // 0x32
	"FBK_COMMA", // 0x33
	"FBK_PERIOD", // 0x34
	"FBK_SLASH", // 0x35
	"FBK_RSHIFT", // 0x36
	"FBK_MULTIPLY", // 0x37
	"FBK_LALT", // 0x38
	"FBK_SPACE", // 0x39
	"FBK_CAPITAL", // 0x3A
	"FBK_F1", // 0x3B
	"FBK_F2", // 0x3C
	"FBK_F3", // 0x3D
	"FBK_F4", // 0x3E
	"FBK_F5", // 0x3F
	"FBK_F6", // 0x40
	"FBK_F7", // 0x41
	"FBK_F8", // 0x42
	"FBK_F9", // 0x43
	"FBK_F10", // 0x44
	"FBK_NUMLOCK", // 0x45
	"FBK_SCROLL", // 0x46
	"FBK_NUMPAD7", // 0x47
	"FBK_NUMPAD8", // 0x48
	"FBK_NUMPAD9", // 0x49
	"FBK_SUBTRACT", // 0x4A
	"FBK_NUMPAD4", // 0x4B
	"FBK_NUMPAD5", // 0x4C
	"FBK_NUMPAD6", // 0x4D
	"FBK_ADD", // 0x4E
	"FBK_NUMPAD1", // 0x4F
	"FBK_NUMPAD2", // 0x50
	"FBK_NUMPAD3", // 0x51
	"FBK_NUMPAD0", // 0x52
	"FBK_DECIMAL", // 0x53
	"", // 0x54
	"", // 0x55
	"FBK_OEM_102", // 0x56
	"FBK_F11", // 0x57
	"FBK_F12",// 0x58
	"", // 0x59
	"", // 0x5A
	"", // 0x5B
	"", // 0x5C
	"", // 0x5D
	"", // 0x5E
	"", // 0x5F
	"", // 0x60
	"", // 0x61
	"", // 0x62
	"", // 0x63
	"FBK_F13", // 0x64
	"FBK_F14", // 0x65
	"FBK_F15", // 0x66
	"", // 0x67
	"", // 0x68
	"", // 0x69
	"", // 0x6A
	"", // 0x6B
	"", // 0x6C
	"", // 0x6D
	"", // 0x6E
	"", // 0x6F
	"FBK_KANA", // 0x70
	"", // 0x71
	"", // 0x72
	"FBK_ABNT_C1", // 0x73
	"", // 0x74
	"", // 0x75
	"", // 0x76
	"", // 0x77
	"", // 0x78
	"FBK_CONVERT", // 0x79
	"", // 0x7A
	"FBK_NOCONVERT", // 0x7B
	"", // 0x7C
	"FBK_YEN", // 0x7D
	"FBK_ABNT_C2", // 0x7E
	"", // 0x7F
	"", // 0x80
	"", // 0x81
	"", // 0x82
	"", // 0x83
	"", // 0x84
	"", // 0x85
	"", // 0x86
	"", // 0x87
	"", // 0x88
	"", // 0x89
	"", // 0x8A
	"", // 0x8B
	"", // 0x8C
	"FBK_NUMPADEQUALS", // 0x8D
	"", // 0x8E
	"", // 0x8F
	"FBK_PREVTRACK", // 0x90
	"FBK_AT", // 0x91
	"FBK_COLON", // 0x92
	"FBK_UNDERLINE", // 0x93
	"FBK_KANJI", // 0x94
	"FBK_STOP", // 0x95
	"FBK_AX", // 0x96
	"FBK_UNLABELED", // 0x97
	"", // 0x98
	"FBK_NEXTTRACK", // 0x99
	"", // 0x9A
	"", // 0x9B
	"FBK_NUMPADENTER", // 0x9C
	"FBK_RCONTROL", // 0x9D
	"", // 0x9E
	"", // 0x9F
	"FBK_MUTE", // 0xA0
	"FBK_CALCULATOR", // 0xA1
	"FBK_PLAYPAUSE", // 0xA2
	"", // 0xA3
	"FBK_MEDIASTOP", // 0xA4
	"", // 0xA5
	"", // 0xA6
	"", // 0xA7
	"", // 0xA8
	"", // 0xA9
	"", // 0xAA
	"", // 0xAB
	"", // 0xAC
	"", // 0xAD
	"FBK_VOLUMEDOWN", // 0xAE
	"", // 0xAF
	"FBK_VOLUMEUP", // 0xB0
	"", // 0xB1
	"FBK_WEBHOME", // 0xB2
	"FBK_NUMPADCOMMA", // 0xB3
	"", // 0xB4
	"FBK_DIVIDE", // 0xB5
	"", // 0xB6
	"FBK_SYSRQ", // 0xB7
	"FBK_RALT", // 0xB8
	"", // 0xB9
	"", // 0xBA
	"", // 0xBB
	"", // 0xBC
	"", // 0xBD
	"", // 0xBE
	"", // 0xBF
	"", // 0xC0
	"", // 0xC1
	"", // 0xC2
	"", // 0xC3
	"", // 0xC4
	"FBK_PAUSE", // 0xC5
	"", // 0xC6
	"FBK_HOME", // 0xC7
	"FBK_UPARROW", // 0xC8
	"FBK_PRIOR", // 0xC9
	"", // 0xCA
	"FBK_LEFTARROW", // 0xCB
	"", // 0xCC
	"FBK_RIGHTARROW", // 0xCD
	"", // 0xCE
	"FBK_END", // 0xCF
	"FBK_DOWNARROW", // 0xD0
	"FBK_NEXT", // 0xD1
	"FBK_INSERT", // 0xD2
	"FBK_DELETE", // 0xD3
	"", // 0xD4
	"", // 0xD5
	"", // 0xD6
	"", // 0xD7
	"", // 0xD8
	"", // 0xD9
	"", // 0xDA
	"FBK_LWIN", // 0xDB
	"FBK_RWIN", // 0xDC
	"FBK_APPS", // 0xDD
	"FBK_POWER", // 0xDE
	"FBK_SLEEP", // 0xDF
	"", // 0xE0
	"", // 0xE1
	"", // 0xE2
	"FBK_WAKE", // 0xE3
	"", // 0xE4
	"FBK_WEBSEARCH", // 0xE5
	"FBK_WEBFAVORITES", // 0xE6
	"FBK_WEBREFRESH", // 0xE7
	"FBK_WEBSTOP", // 0xE8
	"FBK_WEBFORWARD", // 0xE9
	"FBK_WEBBACK", // 0xEA
	"FBK_MYCOMPUTER", // 0xEB
	"FBK_MAIL", // 0xEC
	"FBK_MEDIASELECT", // 0xED
	NULL,
};

int InputFindKey(const char *keystring)
{
	if (strcmp(keystring, "") == 0) {
		return -1;
	}
	
	for (int key = 0; FBK_ALLSTRINGS[key] != NULL; key++) {
		if (strcmp(FBK_ALLSTRINGS[key], keystring) == 0) {
			return key;
		}
	}

	return -1;
}
