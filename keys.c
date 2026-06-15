#include "platform.h"
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
#include "io/elfilewrapper.h"
#include "io/elpathwrapper.h"
#define as_defn(k, s, d) Uint32 k = s;
x_keys(as_defn)
typedef struct Kentry { Uint32 *pval; cstr name; int len; } Kentry;
static Kentry kentries[] = {
	#define as_kentry(k, s, d) { &k, #k+2, sizeof(#k)-3 },
	x_keys(as_kentry)
};
static int cmpke(const void *p, const void *q) {
	Kentry *a = (Kentry *)p, *b = (Kentry *)q;
	int d = a->len - b->len;
	return d ? d : strncasecmp(a->name, b->name, a->len);
}
static inline Kentry *find_kentry(cstr n, int l) {
	if (*n == '#') ++n, --l;
	if (*n == 'K' && n[1] == '_') n += 2, l -= 2;
	Kentry t = {0, n, l};
	return bsearch(&t, kentries, countof(kentries), sizeof(t), cmpke);
}
Uint32 get_key_value(cstr name) {
	Kentry *e = find_kentry(name, strlen(name));
	return e ? *e->pval : 0;
}
#define x_kcodes(x) \
x(UP,273) \
x(F1,282) \
x(F2,283) \
x(F3,284) \
x(F4,285) \
x(F5,286) \
x(F6,287) \
x(F7,288) \
x(F8,289) \
x(F9,290) \
x(F10,291) \
x(F11,292) \
x(F12,293) \
x(F13,294) \
x(F14,295) \
x(F15,296) \
x(BACKSPACE,8) \
x(TAB,9) \
x(CLEAR,12) \
x(RETURN,13) \
x(PAUSE,19) \
x(ESCAPE,27) \
x(SPACE,32) \
x(DELETE,127) \
x(KP0,256) \
x(KP1,257) \
x(KP2,258) \
x(KP3,259) \
x(KP4,260) \
x(KP5,261) \
x(KP6,262) \
x(KP7,263) \
x(KP8,264) \
x(KP9,265) \
x(KP_PERIOD,266) \
x(KP_DIVIDE,267) \
x(KP_MULTIPLY,268) \
x(KP_MINUS,269) \
x(KP_PLUS,270) \
x(KP_ENTER,271) \
x(KP_EQUALS,272) \
x(DOWN,274) \
x(RIGHT,275) \
x(LEFT,276) \
x(INSERT,277) \
x(HOME,278) \
x(END,279) \
x(PAGEUP,280) \
x(PAGEDOWN,281) \
x(NUMLOCK,300) \
x(CAPSLOCK,301) \
x(SCROLLOCK,302) \
x(RSHIFT,303) \
x(LSHIFT,304) \
x(RCTRL,305) \
x(LCTRL,306) \
x(RALT,307) \
x(LALT,308) \
x(RMETA,309) \
x(LMETA,310) \
x(LSUPER,311) \
x(RSUPER,312) \
x(MODE,313) \
x(COMPOSE,314) \
x(HELP,315) \
x(PRINT,316) \
x(SYSREQ,317) \
x(BREAK,318) \
x(MENU,319) \
x(POWER,320) \
x(EURO,321) \
x(UNDO,322)
typedef struct Kcode { cstr name; int len; int val; } Kcode;
static Kcode kcodes[] = {
	#define as_kcode(n,v) {#n, sizeof(#n)-1, v},
	x_kcodes(as_kcode)
};
static cstr kctab[512];
static int cmpkc(const void *p, const void *q) {
	Kcode *a = (Kcode *)p, *b = (Kcode *)q;
	int d = a->len - b->len;
	return d ? d : strncasecmp(a->name, b->name, a->len);
}
static int get_kcode(cstr s, int n) {
	if (n == 1) {
		return tolower(s[0]);
	}
	Kcode t = {s, n}, *r;
	r = bsearch(&t, kcodes, countof(kcodes), sizeof(t), cmpkc);
	return r ? r->val : SDLK_UNKNOWN;
}
static inline Uint32 update_key(Uint32 k, Uint32 v) {
	if (v == 300 || v == 304) {
		k |= SHIFT;
	} else if (v == 305 || v == 306) {
		k |= CTRL;
	} else if (v == 307 || v == 308) {
		k |= ALT;
	} else {
		k = (v & 0xffff) | (k & 0xffff0000);
	}
	return k;
}
typedef struct Parser { cstr path, line, c; int ln; } Parser;
static inline int cspace(char c) { return c == ' ' || c == '\t'; }
static inline int cword(char c) { return c && c != ' ' && c != '\t' && c != '\r' && c != '\n'; }
static inline void skip_whitespace(Parser *p) {
	for (; cspace(*p->c); ++p->c);
}
static inline int consume(Parser *p, cstr w) {
	for (; *p->c && *w && *p->c == *w; ++p->c, ++w);
	return !*w;
}
static inline cstr get_word(Parser *p, int *out_len) {
	cstr r = p->c;
	for (; cword(*p->c); ++p->c);
	*out_len = p->c - r;
	return *out_len ? r : 0;
}
static inline int more(Parser *p) { return *p->c && *p->c != '\r' && *p->c != '\n'; }
static inline void next_line(Parser *p, cstr line) {
	p->c = p->line = line;
	++p->ln;
}
#define warn(f, ...) LOG_ERROR("%s line %d: " f, p->path, p->ln, ##__VA_ARGS__)
static void parse_key(Parser *p) {
	skip_whitespace(p);
	if (!consume(p,  "#K_")) {
		return;
	}
	int c, knl, wl;
	cstr kn = get_word(p, &knl), w;
	skip_whitespace(p);
	if (!consume(p, "=")) {
		warn("expecting '='");
		return;
	}
	Uint32 k = 0;
	while (more(p)) {
		skip_whitespace(p);
		if ((w = get_word(p, &wl))) {
			c = get_kcode(w, wl);
			k = update_key(k, c);
		}
	}
	if (k) {
		Kentry *e = find_kentry(kn, knl);
		if (e) {
			*e->pval = k;
		} else {
			warn("unrecognized key name \"%.*s\"", knl, kn);
		}
	} else {
		warn("invalid syntax \"%s\"", p->line);
	}
}
static void init_keydata(void) {
	#define as_reset_keys(k,s,d) k = s;
	x_keys(as_reset_keys);
	qsort(kentries, countof(kentries), sizeof(*kentries), cmpke);
	qsort(kcodes, countof(kcodes), sizeof(*kcodes), cmpkc);
	#define as_kctab(n, c) kctab[c] = #n;
	x_kcodes(as_kctab)
}
void read_key_config(void) {
	init_keydata();
	cstr path = "key.ini";
#ifdef LINUX
	path = "key_linux.ini";
#endif
	el_file_ptr f = el_open_custom(path);
	if (f) {
		char l[512], r[2048];
		Parser _p = {path}, *p = &_p;
		while (el_fgets(l, sizeof(l), f)) {
			next_line(p, l);
			parse_key(p);
		}
		cprintf(c_green4, "Les raccourcis clavier ont été chargés de %s", realpath(el_file_name(f), r));
		el_close(f);
	} else {
		LOG_ERROR("el_open_custom failed for key config path \"%s\"", path);
	}
	Uint32 v = SDLK_LAST;
	#define as_init(k, s, d) if (!k) k = ++v;
	x_keys(as_init)
}
cstr get_key_string(Uint32 k, char *buf, size_t buflen) {
	Uint32 b = k & ~(CTRL | ALT | SHIFT);
	cstr c = k & CTRL ? "ctrl-" : "";
	cstr a = k & ALT ? "alt-" : "";
	cstr s = k & SHIFT ? "shift-" : "";
	cstr d = b < countof(kctab) ? kctab[b] : 0;
	char t[16] = {0};
	if (!d) {
		if (32 < b && b < 127) {
			t[0] = 'A' <= b && b <= 'Z' ? b | 32 : b;
		} else if (!b || b > SDLK_LAST) {
			safe_snprintf(t, sizeof(t), "(aucun)");
		} else {
			safe_snprintf(t, sizeof(t), "\\x%04x", b);
		}
		d = t;
	}
	safe_snprintf(buf, buflen, "%s%s%s%s", c, a, s, d);
	return buf;
}
