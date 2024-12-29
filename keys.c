#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <SDL_keysym.h>
#include "errors.h"
#include "keys.h"
#include "asc.h"
#include "init.h"
#include "misc.h"
#ifdef FASTER_MAP_LOAD
#include "io/elfilewrapper.h"
#endif
#ifndef ENGLISH
#include "io/elpathwrapper.h"
#endif //ENGLISH

#define as_defn(k, s, d) Uint32 k = s;
x_keys(as_defn)
#undef as_defn

typedef struct
{
	char name[25];
	Uint32 *value;
} key_store_entry;

static key_store_entry key_store[] = {
	#define as_entry(k, s, d) { #k, &k },
	x_keys(as_entry)
	#undef as_entry
};


Uint32 get_key_value(const char* name)
{
	size_t num_keys = sizeof(key_store)/sizeof(key_store_entry);
	size_t i;
	if ((name == NULL) || strlen(name) == 0)
	{
		LOG_ERROR("%s() empty name\n", __FUNCTION__);
		return 0;
	}
	for (i=0; i<num_keys; i++)
	{
		if (strcasecmp(name, key_store[i].name) == 0)
			return *key_store[i].value;
	}
	return 0;
}

static void add_key(Uint32 *key, Uint32 n)
{
	switch (n)
	{
		case 303:
		case 304:
			*key |= SHIFT;
			break;
		case 305:
		case 306:
			*key |= CTRL;
			break;
		case 307:
		case 308:
			*key |= ALT;
			break;
		default:
			*key = (n & 0xFFFF) | (*key &0xFFFF0000);
        }
}

static Uint32 CRC32(const char *data, int len)
{
	unsigned int result = 0;
	int i, j;
	unsigned char octet;

	for (i = 0; i < len; i++)
	{
		octet = *(data++);
		for (j = 0; j < 8; j++)
		{
			if ((octet >> 7) ^ (result >> 31))
				result = (result << 1) ^ 0x04c11db7;
			else
				result = (result << 1);
			octet <<= 1;
		}
	}

	return ~result;
}

static Uint16 get_key_code(const char *key)
{
	int len = strlen(key);

	if (len==1)
	{
			return tolower(key[0]);
	}
	else
	{
		Uint32 crc = CRC32(key,len);
		switch(crc){
			case 0x414243d2: //UP
				return 273;
			case 0x8b9c5c32: //F1
				return 282;
			case 0x86df7aeb: //F2
				return 283;
			case 0x821e675c: //F3
				return 284;
			case 0x9c593759: //F4
				return 285;
			case 0x98982aee: //F5
				return 286;
			case 0x95db0c37: //F6
				return 287;
			case 0x911a1180: //F7
				return 288;
			case 0xa955ac3d: //F8
				return 289;
			case 0xad94b18a: //F9
				return 290;
			case 0xbbde3454: //F10
				return 291;
			case 0xbf1f29e3: //F11
				return 292;
			case 0xb25c0f3a: //F12
				return 293;
			case 0xb69d128d: //F13
				return 294;
			case 0xa8da4288: //F14
				return 295;
			case 0xac1b5f3f: //F15
				return 296;
			case 0xe5b332af: //BACKSPACE
				return 8;
			case 0x3d6742da: //TAB
				return 9;
			case 0xe4f512ce: //CLEAR
				return 12;
			case 0xe5c642f: //RETURN
				return 13;
			case 0x1a3dbcf4: //PAUSE
				return 19;
			case 0xb23e322f: //ESCAPE
				return 27;
			case 0xe0ea4208: //SPACE
				return 32;
			case 0x3f048816: //DELETE
				return 127;
			case 0x5dd541: //KP0
				return 256;
			case 0x49cc8f6: //KP1
				return 257;
			case 0x9dfee2f: //KP2
				return 258;
			case 0xd1ef398: //KP3
				return 259;
			case 0x1359a39d: //KP4
				return 260;
			case 0x1798be2a: //KP5
				return 261;
			case 0x1adb98f3: //KP6
				return 262;
			case 0x1e1a8544: //KP7
				return 263;
			case 0x265538f9: //KP8
				return 264;
			case 0x2294254e: //KP9
				return 265;
			case 0xc9681663: //KP_PERIOD
				return 266;
			case 0xf2032002: //KP_DIVIDE
				return 267;
			case 0xc69c9177: //KP_MULTIPLY
				return 268;
			case 0xe05a3b75: //KP_MINUS
				return 269;
			case 0x7a14ede0: //KP_PLUS
				return 270;
			case 0xb95fb1fa: //KP_ENTER
				return 271;
			case 0x997d27b6: //KP_EQUALS
				return 272;
			case 0x412c789a: //DOWN
				return 274;
			case 0xcfd43bcf: //RIGHT
				return 275;
			case 0x14618acf: //LEFT
				return 276;
			case 0xb448467c: //INSERT
				return 277;
			case 0xd59321ba: //HOME
				return 278;
			case 0x863456b7: //END
				return 279;
			case 0xd541afe1: //PAGEUP
				return 280;
			case 0x77a53c61: //PAGEDOWN
				return 281;
			case 0x8563dfd4: //NUMLOCK
				return 300;
			case 0x4b601de5: //CAPSLOCK
				return 301;
			case 0x7b642f: //SCROLLOCK
				return 302;
			case 0x6fa8765e: //RSHIFT
				return 303;
			case 0x5a59f8b9: //LSHIFT
				return 304;
			case 0xc535c663: //RCTRL
				return 305;
			case 0xb5e083f0: //LCTRL
				return 306;
			case 0xf7a834fb: //RALT
				return 307;
			case 0x39b9e58e: //LALT
				return 308;
			case 0x34796737: //RMETA
				return 309;
			case 0x44ac22a4: //LMETA
				return 310;
			case 0x8ec5890c: //LSUPER
				return 311;
			case 0xbb3407eb: //RSUPER
				return 312;
			case 0x2d5a7586: //MODE
				return 313;
			case 0x87140862: //COMPOSE
				return 314;
			case 0x512a6d4b: //HELP
				return 315;
			case 0xdc87c39e: //PRINT
				return 316;
			case 0xbdf2d984: //SYSREQ
				return 317;
			case 0xd318f49: //BREAK
				return 318;
			case 0x46854e9d: //MENU
				return 319;
			case 0x8758b6ec: //POWER
				return 320;
			case 0x1e43eaa9: //EURO
				return 321;
			case 0xdf6ba7e: //UNDO
				return 322;
			default:
				return SDLK_UNKNOWN;
		}
	}
}

static void parse_key_line(const char *line)
{
	char kstr[100], t1[100], t2[100], t3[100], t4[100];
	Uint32 key = 0;
	int nkey = sscanf(line, " #K_%99s = %99s %99s %99s %99s", kstr,
		t1, t2, t3, t4);
	size_t num_keys = sizeof(key_store)/sizeof(key_store_entry);
	size_t i;

	if (nkey <= 1)
		return;

	add_key(&key, get_key_code(t1));
	if (nkey > 2 && t2[0] != '#')
	{
		add_key(&key, get_key_code(t2));
		if (nkey > 3 && t3[0] != '#')
		{
			add_key(&key, get_key_code(t3));
			if (nkey > 4 && t4[0] != '#')
			{
				add_key(&key, get_key_code(t4));
			}
		}
	}

	for (i=0; i<num_keys; i++)
		if (strcasecmp(kstr, &key_store[i].name[3]) == 0) // skip "#K_"
		{
			*key_store[i].value = key;
			break;
		}
}

// load the dynamic definitions for keys
void read_key_config()
{
	char line[512];
	el_file_ptr f;
	size_t num_keys = sizeof(key_store)/sizeof(key_store_entry);
	Uint32 last_key_value = SDLK_LAST;
	size_t i;

#ifdef FR_VERSION
#ifdef LINUX
	f = el_open_custom("key_linux.ini");
#else //LINUX
	f = el_open_custom("key.ini");
#endif //LINUX
#else //FR_VERSION
	f = el_open_custom("key.ini");
#endif //FR_VERSION
	if (f)
	{
		while (el_fgets(line, sizeof(line), f))
		{
			parse_key_line(line);
		}
		el_close(f);
	}

	// look for unassigned keys and assign one up from SDLK_LAST
	for (i=0; i<num_keys; i++)
		if (*key_store[i].value == 0)
			*key_store[i].value = ++last_key_value;
}

// Returns (in the buffer provided) a string describing the specified keydef.
const char *get_key_string(Uint32 keydef, char *buf, size_t buflen)
{
	char base = keydef & 0xFF;
	char *mod = "";
	if (keydef & CTRL)
		mod = "ctrl-";
	else if (keydef & ALT)
		mod = "alt-";
	else if (keydef & SHIFT)
		mod = "shift-";
	safe_snprintf(buf, buflen, "%s%c", mod, base);
	return buf;
}
