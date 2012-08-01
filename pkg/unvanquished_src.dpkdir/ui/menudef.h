#ifndef MENUDEF_H
#define MENUDEF_H

enum
{
  ITEM_TYPE_ANY = -1, // invalid type
  ITEM_TYPE_NONE, // no specified type
  ITEM_TYPE_TEXT, // simple text
  ITEM_TYPE_BUTTON, // button, basically text with a border
  ITEM_TYPE_RADIOBUTTON, // toggle button, may be grouped
  ITEM_TYPE_CHECKBOX, // check box
  ITEM_TYPE_EDITFIELD, // editable text, associated with a cvar
  ITEM_TYPE_SAYFIELD, // the chat field
  ITEM_TYPE_CYCLE, // cycling list
  ITEM_TYPE_LISTBOX, // scrollable list
  ITEM_TYPE_COMBOBOX, // drop down scrollable list
  ITEM_TYPE_MODEL, // model
  ITEM_TYPE_OWNERDRAW, // owner draw, has an associated ownerdraw number
  ITEM_TYPE_NUMERICFIELD, // editable text, associated with a cvar
  ITEM_TYPE_SLIDER, // mouse speed, volume, etc.
  ITEM_TYPE_YESNO, // boolean cvar setting
  ITEM_TYPE_MULTI, // multiple list setting, enumerated
  ITEM_TYPE_BIND // keyboard control configuration
};

// The first items in these enums are the defaults if menus do not specify
enum
{
  ALIGN_LEFT, // left alignment
  ALIGN_CENTER, // center alignment
  ALIGN_RIGHT, // right alignment
  ASPECT_NONE, // no aspect compensation
  ALIGN_NONE
};

enum
{
  VALIGN_BOTTOM, // bottom alignment
  VALIGN_CENTER, // center alignment
  VALIGN_TOP, // top alignment
  VALIGN_NONE
};

enum
{
  ITEM_TEXTSTYLE_PLAIN, // plain
  ITEM_TEXTSTYLE_BLINK, // fast blinking
  ITEM_TEXTSTYLE_PULSE, // slow pulsing
  ITEM_TEXTSTYLE_SHADOWED, // drop shadow (need a color for this)
  ITEM_TEXTSTYLE_OUTLINED, // apparently unimplemented
  ITEM_TEXTSTYLE_OUTLINESHADOWED, // apparently unimplemented
  ITEM_TEXTSTYLE_SHADOWEDMORE, // drop shadow (need a color for this)
  ITEM_TEXTSTYLE_NEON // glow (need a color for this)
};
#define ITEM_TEXTSTYLE_NORMAL ITEM_TEXTSTYLE_PLAIN // deprecated, provided for compatibility

enum
{
  WINDOW_BORDER_NONE, // no border
  WINDOW_BORDER_FULL, // full border based on border color (single pixel)
  WINDOW_BORDER_HORZ, // horizontal borders only
  WINDOW_BORDER_VERT, // vertical borders only
  WINDOW_BORDER_KCGRADIENT // horizontal border using the gradient bars
};

enum
{
  WINDOW_STYLE_EMPTY, // no background
  WINDOW_STYLE_FILLED, // filled with background color
  WINDOW_STYLE_GRADIENT, // gradient bar based on background color
  WINDOW_STYLE_SHADER, // use background shader
  WINDOW_STYLE_TEAMCOLOR, // team color
  WINDOW_STYLE_CINEMATIC // cinematic
};

#define MENU_TRUE  1 // uh.. true
#define MENU_FALSE 0 // and false

enum
{
  HUD_VERTICAL,
  HUD_HORIZONTAL
};

// list box element types
enum
{
  LISTBOX_TEXT,
  LISTBOX_IMAGE
};

// list feeders
enum
{
  FEEDER_SERVERS, // servers
  FEEDER_MAPS, // all maps available, in graphic format
  FEEDER_ALIENTEAM_LIST, // alien team members
  FEEDER_HUMANTEAM_LIST, // human team members
  FEEDER_TEAM_LIST, // team members for team voting
  FEEDER_PLAYER_LIST, // players
  FEEDER_NEWS, // news
  FEEDER_MODS, // list of available mods
  FEEDER_DEMOS, // list of available demo files
  FEEDER_SERVERSTATUS, // server status
  FEEDER_FINDPLAYER, // find player
  FEEDER_CINEMATICS, // cinematics

  FEEDER_TREMTEAMS, // teams
  FEEDER_TREMALIENCLASSES, // alien classes
  FEEDER_TREMHUMANITEMS, // human items
  FEEDER_TREMHUMANARMOURYBUY, // human buy
  FEEDER_TREMHUMANARMOURYSELL, // human sell
  FEEDER_TREMALIENUPGRADE, // alien upgrade
  FEEDER_TREMALIENBUILD, // alien buildables
  FEEDER_TREMHUMANBUILD, // human buildables
  FEEDER_IGNORE_LIST, // ignored players
  FEEDER_HELP_LIST, // help topics
  FEEDER_RESOLUTIONS, // display resolutions
  FEEDER_PROFILES, // Profiles
  FEEDER_LANGUAGES // Languages
};

// display flags
#define UI_SHOW_FAVORITESERVERS    0x00000001
#define UI_SHOW_NOTFAVORITESERVERS 0x00000002

#define UI_SHOW_VOTEACTIVE         0x00000004
#define UI_SHOW_CANVOTE            0x00000008
#define UI_SHOW_TEAMVOTEACTIVE     0x00000010
#define UI_SHOW_CANTEAMVOTE        0x00000020

#define UI_SHOW_NOTSPECTATING      0x00000040

// owner draw types
enum
{
  CG_PLAYER_HEALTH,
  CG_PLAYER_HEALTH_BAR,
  CG_PLAYER_HEALTH_CROSS,
  CG_PLAYER_HEALTH_METER,
  CG_PLAYER_AMMO_VALUE,
  CG_PLAYER_CLIPS_VALUE,
  CG_PLAYER_CLIPS_METER,
  CG_PLAYER_BUILD_TIMER,
  CG_PLAYER_BUILD_TIMER_BAR,
  CG_PLAYER_CREDITS_VALUE,
  CG_PLAYER_CREDITS_VALUE_NOPAD,
  CG_PLAYER_ALIEN_EVOS,
  CG_PLAYER_STAMINA,
  CG_PLAYER_STAMINA_1,
  CG_PLAYER_STAMINA_2,
  CG_PLAYER_STAMINA_3,
  CG_PLAYER_STAMINA_4,
  CG_PLAYER_STAMINA_BOLT,
  CG_PLAYER_STAMINA_VALUE,
  CG_PLAYER_BOOST_BOLT,
  CG_PLAYER_CLIPS_RING,
  CG_PLAYER_BUILD_TIMER_RING,
  CG_PLAYER_SELECT,
  CG_PLAYER_SELECTTEXT,
  CG_PLAYER_WEAPONICON,
  CG_PLAYER_WALLCLIMBING,
  CG_PLAYER_BOOSTED,
  CG_PLAYER_BOOSTED_BAR,
  CG_PLAYER_POISON_BARBS,
  CG_PLAYER_ALIEN_SENSE,
  CG_PLAYER_HUMAN_SCANNER,
  CG_PLAYER_USABLE_BUILDABLE,
  CG_PLAYER_CHARGE_BAR_BG,
  CG_PLAYER_CHARGE_BAR,
  CG_PLAYER_CROSSHAIR,
  CG_PLAYER_LOCATION,
  CG_TEAMOVERLAY,
  CG_PLAYER_CREDITS_FRACTION,
  CG_PLAYER_AMMO_STACK,
  CG_PLAYER_CLIPS_STACK,
  CG_PLAYER_TOTAL_AMMO_VALUE,

  CG_KILLER,
  CG_SPECTATORS,
  CG_FOLLOW,

// loading screen
  CG_LOAD_LEVELSHOT,
  CG_LOAD_MEDIA,
  CG_LOAD_MEDIA_LABEL,
  CG_LOAD_BUILDABLES,
  CG_LOAD_BUILDABLES_LABEL,
  CG_LOAD_CHARMODEL,
  CG_LOAD_CHARMODEL_LABEL,
  CG_LOAD_OVERALL,
  CG_LOAD_LEVELNAME,
  CG_LOAD_MOTD,
  CG_LOAD_HOSTNAME,

  CG_FPS,
  CG_FPS_FIXED,
  CG_TIMER,
  CG_TIMER_MINS,
  CG_TIMER_SECS,
  CG_SNAPSHOT,
  CG_LAGOMETER,
  CG_SPEEDOMETER,
  CG_PLAYER_CROSSHAIRNAMES,
  CG_STAGE_REPORT_TEXT,
  CG_ALIENS_SCORE_LABEL,
  CG_HUMANS_SCORE_LABEL,
  CG_DEMO_PLAYBACK,
  CG_DEMO_RECORDING,

  CG_CONSOLE,
  CG_TUTORIAL,
  CG_CLOCK,

  UI_NETSOURCE,
  UI_NETMAPPREVIEW,
  UI_NETMAPCINEMATIC,
  UI_SERVERREFRESHDATE,
  UI_SERVERMOTD,
  UI_GLINFO,
  UI_KEYBINDSTATUS,
  UI_SELECTEDMAPPREVIEW,
  UI_SELECTEDMAPNAME,

  UI_TEAMINFOPANE,
  UI_ACLASSINFOPANE,
  UI_AUPGRADEINFOPANE,
  UI_HITEMINFOPANE,
  UI_HBUYINFOPANE,
  UI_HSELLINFOPANE,
  UI_ABUILDINFOPANE,
  UI_HBUILDINFOPANE,
  UI_HELPINFOPANE
};

// Define some colors
#define MENU_TEAL       0 .1 .2
#define MENU_TEAL_TRANS MENU_TEAL .9


#endif
