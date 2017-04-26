#define DEBUG			1

#define NINETERM_ARGS	"-f", "/usr/local/plan9/font/vga/vga.font", nil
#define DMENU_ARGS		"-fn", "terminus", nil 

#define CMDTERM		{ "9term", NINETERM_ARGS }
#define CMDNEW		{ "urxvtc", nil }
#define CMDRUN		{ "dmenu_run", DMENU_ARGS }

#define RESIZEONMAPR	{	\
	"9term",	\
	"acme",		\
	"djview",	\
	"mupdf",	\
	"rxvt",		\
	"sam",		\
	"urxvt",	\
	"xterm",	\
	nil,		\
}

#define FREEBSD_THEME 1

#if FREEBSD_THEME == 1

# undef DMENU_ARGS
# define DMENU_ARGS "-fn", "xos4 Terminus", "-nf", "#BA0B0B", "-sb", "#690000", "-sf", "#EB0202", nil

# define ROOTBG		0xFF161616
# define ACTIVEBR	0xFFEB0202
# define INACTIVEBR	0xFF690000
# define BLANKBG		0xFF000000
# define BLANKBR		0xFFAA0000
# define TRANSBR		0xFFBB0000
# define TRANSBG		0x20000000

# define MENUFACE		"-*-terminus-*-*-*-*-14-*-*-*-*-*-*-*"
# define MENUBR			0xFF970808
# define MENUTXTFG_S		0xFFEB0202
# define MENUTXTFG_U		0xFF000000
# define MENUSLOTFG_S	0xFF220000
# define MNUSLOTFG_U		0xFFBA0B0B

#else 

/* PLAN 9 Style */

# define ROOTBG		0xFF808080
# define ACTIVEBR	0xFF57A8A8
# define INACTIVEBR	0xFF9FEFEF
# define BLANKBG		0xFFEFEFEF
# define BLANKBR		0xFFAA0000
# define TRANSBR		0xFFBB0000
# define TRANSBG		0x20000000

# define MENUFACE		"-*-terminus-*-*-*-*-14-*-*-*-*-*-*-*"
# define MENUBR			0xFF88CC88
# define MENUTXTFG_S		0xFFEAFFEA
# define MENUTXTFG_U		0xFF000000
# define MENUSLOTFG_S	0xFF448844
# define MNUSLOTFG_U		0xFFEAFFEA

#endif
