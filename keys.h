/*!
 * \file
 * \ingroup hotkey
 * \brief Handling of hotkeys.
 */
#ifndef __KEYS_H__
#define __KEYS_H__

#include <SDL_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \name Key Modifiers
 */
/*! @{ */
#define SHIFT (1u << 31)   /*!< Shift modifier is pressed */
#define CTRL (1u << 30)    /*!< Ctrl modifier is pressed */
#define ALT (1u << 29)     /*!< Alt modifier is pressed */
/*! @} */

#define x_keys(x) \
x(K_QUIT, 0, "Fermeture du jeu") \
x(K_QUIT_ALT, 0, "Fermeture du jeu (alternatif)") \
x(K_CAMERAUP, SDLK_UP, "Rotation caméra vers le haut") \
x(K_CAMERADOWN, SDLK_DOWN, "Rotation caméra vers le bas") \
x(K_ZOOMOUT, SDLK_PAGEDOWN, "Zoom arrière") \
x(K_ZOOMIN, SDLK_PAGEUP, "Zoom avant") \
x(K_TURNLEFT, SDLK_LEFT, "Rotation personnage vers la gauche") \
x(K_TURNRIGHT, SDLK_RIGHT, "Rotation personnage vers la droite") \
x(K_ADVANCE, SDLK_HOME, "Avancement, ou fuite pendant le combat") \
x(K_HEALTHBAR, ALT|'h', "Barre de vie") \
x(K_VIEWNAMES, ALT|'n', "Affichage des noms") \
x(K_VIEWHP, ALT|'b', "Points de vie") \
x(K_VIEWMP, ALT|'k', "Points de mana") \
x(K_VIEWMANABAR, ALT|'l', "Barre de mana") \
x(K_STATS, CTRL|'a', "Fenêtre statistiques") \
x(K_QUESTLOG, ALT|'q', "Fenêtre quêtes") \
x(K_SESSION, CTRL|'z', "Fenêtre session") \
x(K_WALK, CTRL|'w', "Curseur se déplacer") \
x(K_LOOK, CTRL|'l', "Curseur observer") \
x(K_USE, CTRL|'u', "Curseur utiliser") \
x(K_OPTIONS, CTRL|'o', "Fenêtre options") \
x(K_REPEATSPELL, CTRL|'r', "Répétition de sort") \
x(K_REPEATDIALOG, ALT|'y', "Répétition de réponse dialogue") \
x(K_MIXONE, SDLK_F3, "Production à l'unité") \
x(K_MIXALL, SDLK_F4, "Production en série") \
x(K_DROPALL, SDLK_F5, "Tout au sol") \
x(K_STOREALL, SDLK_F8, "Tout au dépôt") \
x(K_WITHDRAW, SDLK_F9, "Activation de la liste d'objets") \
x(K_SIGILS, CTRL|'s', "Fenêtre grimoire") \
x(K_MANUFACTURE, CTRL|'m', "Fenêtre fabrication") \
x(K_ITEMS, CTRL|'i', "Fenêtre inventaire") \
x(K_MAP, SDLK_TAB, "Fenêtre carte") \
x(K_MINIMAP, ALT|'t', "Affichage minicarte") \
x(K_ROTATELEFT, CTRL|SDLK_LEFT, "Rotation caméra gauche") \
x(K_ROTATERIGHT, CTRL|SDLK_RIGHT, "Rotation caméra droite") \
x(K_FROTATELEFT, CTRL|SHIFT|SDLK_LEFT, "Rotation caméra précise gauche") \
x(K_FROTATERIGHT, CTRL|SHIFT|SDLK_RIGHT, "Rotation caméra précise droite") \
x(K_BROWSER, ALT|SDLK_F2, "Activation navigateur internet") \
x(K_BROWSERWIN, CTRL|SDLK_F2, "Fenêtre des liens internet") \
x(K_CONSOLE, SDLK_F1, "Fenêtre historique") \
x(K_SHADOWS, CTRL|SDLK_F3, "Affichage des ombres") \
x(K_KNOWLEDGE, CTRL|'k', "Fenêtre connaissances") \
x(K_ENCYCLOPEDIA, CTRL|'e', "Fenêtre encyclopé") \
x(K_HELP, CTRL|'h', "Fenêtre aide") \
x(K_RULES, CTRL|SDLK_F5, "Fenêtre règles") \
x(K_NOTEPAD, CTRL|'n', "Fenêtre bloc-notes") \
x(K_HIDEWINS, ALT|'d', "Masquer/afficher toutes les fenêtres ouvertes") \
x(K_SCREENSHOT, CTRL|'p', "Capture d'écran") \
x(K_VIEWTEXTASOVERTEXT, ALT|'o', "Affichage des boules de dialogues") \
x(K_AFK, CTRL|ALT|'a', "Activation message APM automatique") \
x(K_SIT, ALT|'s', "Personnage assis ou debout") \
x(K_BUDDY, CTRL|'b', "Fenêtre des aventuriers connus") \
x(K_NEXT_CHAT_TAB, CTRL|SDLK_PAGEDOWN, "Onglet discussion prochain") \
x(K_PREV_CHAT_TAB, CTRL|SDLK_PAGEUP, "Onglet discussion précédent") \
x(K_TABCOMPLETE, CTRL|' ', "Auto-complétion des commandes ou noms") \
x(K_WINDOWS_ON_TOP, ALT|'w', "Fenêtres en évidence") \
x(K_MARKFILTER, CTRL|'f', "Filtre marques des cartes") \
x(K_OPAQUEWIN, CTRL|'d', "Fonds de fenêtres opaques") \
x(K_GRAB_MOUSE, ALT|'g', "Contrôle souris caméra ou IHM") \
x(K_FIRST_PERSON, ALT|'f', "Point de vue du personnage") \
x(K_EXTEND_CAM, ALT|'e', "Caméra étendu") \
x(K_CUT, CTRL|'x', "Couper texte au presse-papier") \
x(K_COPY, CTRL|'c', "Copier texte au presse-papier") \
x(K_PASTE, CTRL|'v', "Coller texte du presse-papier") \
x(K_COPY_ALT, CTRL|SDLK_INSERT, "Copier texte (alternatif)") \
x(K_PASTE_ALT, SHIFT|SDLK_INSERT, "Coller texte (alternatif)") \
x(K_ECDEBUGWIN, 0, "Fenêtre debug eye candy") \
x(K_INCUNABLES, CTRL|'g', "Fenêtre incunables") \
x(K_OBTENIR, ALT|'r', "Ouvrir et ramasser un sac") \
x(K_VOIR_MUSIQUE_CARTE, ALT|'z', "Voir les zones des musiques sur les cartes") \
x(K_FENETRE_MUSIQUE, ALT|'m', "Fenêtre musique") \
x(K_ITEM1, CTRL|'1', "Barre rapide objet 1") \
x(K_ITEM2, CTRL|'2', "Barre rapide objet 2") \
x(K_ITEM3, CTRL|'3', "Barre rapide objet 3") \
x(K_ITEM4, CTRL|'4', "Barre rapide objet 4") \
x(K_ITEM5, CTRL|'5', "Barre rapide objet 5") \
x(K_ITEM6, CTRL|'6', "Barre rapide objet 6") \
x(K_ITEM7, CTRL|'7', "Barre rapide objet 7") \
x(K_ITEM8, CTRL|'8', "Barre rapide objet 8") \
x(K_ITEM9, CTRL|'9', "Barre rapide objet 9") \
x(K_ITEM10, CTRL|'0', "Barre rapide objet 10") \
x(K_ITEM11, CTRL|')', "Barre rapide objet 11") \
x(K_ITEM12, CTRL|'=', "Barre rapide objet 12") \
x(K_SPELL1, ALT|'1', "Barre sort 1") \
x(K_SPELL2, ALT|'2', "Barre sort 2") \
x(K_SPELL3, ALT|'3', "Barre sort 3") \
x(K_SPELL4, ALT|'4', "Barre sort 4") \
x(K_SPELL5, ALT|'5', "Barre sort 5") \
x(K_SPELL6, ALT|'6', "Barre sort 6") \
x(K_SPELL7, ALT|'7', "Barre sort 7") \
x(K_SPELL8, ALT|'8', "Barre sort 8") \
x(K_SPELL9, ALT|'9', "Barre sort 9") \
x(K_SPELL10, ALT|'0', "Barre sort 10") \
x(K_SPELL11, ALT|'-', "Barre sort 11") \
x(K_SPELL12, ALT|'=', "Barre sort 12") \
x(K_PREVQUICKSPELLBAR, ALT|SDLK_LEFT, "Barre des sorts prochaine") \
x(K_NEXTQUICKSPELLBAR, ALT|SDLK_RIGHT, "Barre des sorts précédente") \
x(K_SPELLTARGET, ALT|'c', "Ciblage automatique des sorts") \
x(K_SPELLSELF, ALT|'j', "Lancer un sort sur soi-même")

#define as_decl(k, s, d) extern Uint32 k;
x_keys(as_decl)
#undef as_decl

/*!
 * \ingroup loadsave
 * \brief   Reads the key configuration from the default key.ini file.
 *
 *      Reads the shortcut key configuration from the default key.ini file.
 *
 * \callgraph
 */
void read_key_config();


/*!
 * \brief   Returns a string describing the specified keydef.
 *
 *      Returns (in the buffer provided) a string describing the specified keydef.
 *
 * \callgraph
 */
const char *get_key_string(Uint32 keydef, char *buf, size_t buflen);

/*!
 * \brief   Returns the value of the specified keydef.
 *
 *      Returns the key value or 0 if not found.
 *
 * \callgraph
 */
Uint32 get_key_value(const char* name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif	//__KEYS_H__
