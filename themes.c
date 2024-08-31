// #include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include "asc.h"
#include "dialogues.h"
#include "elconfig.h"
#include "errors.h"
#include "io/elpathwrapper.h"
#include "init.h"
#include "translate.h"
#include "colors.h"
#include "fr_quickitems.h"
#include "spells.h"
#include "themes.h"
#include "hud.h"
theme liste_themes[LISTE_THEMES_MAX]; // liste des thèmes trouvés (updates + datadir)
int liste_themes_nb = 0; // nombre d'éléments dans la liste des thèmes
char titre_theme[20] = "officiel"; // titre du thème à utiliser par défaut
/*******************************************************************************
 * Fonctions de conversion d'une chaine vers un nombre
 */
void strtod_securise(const char *valeur, float *variable) {
	char *PosErreur;
	float resultat;
	char *temp;
	int trim_effectue = 0;
	if ((valeur[0] == ' ') || (valeur[strlen(valeur)] == ' ')) {
		temp = strdup(valeur);
		resultat = strtod(trim(temp), &PosErreur);
		trim_effectue = 1;
	} else {
		resultat = strtod(valeur, &PosErreur);
	}
	// La commande precedente n'a pas renvoyer d'erreur
	if (*PosErreur == '\0') {
		*variable = (float)resultat;
	} else {
		LOG_ERROR("La balise ne contient pas de chiffre !\n");
	}
	if (trim_effectue == 1) {
		free(temp);
	}
}
void strtol_securise(const char *valeur, int *variable) {
	char *PosErreur;
	long resultat;
	char *temp;
	int trim_effectue = 0;
	if ((valeur[0] == ' ') || (valeur[strlen(valeur)] == ' ')) {
		temp = strdup(valeur);
		resultat = strtol(trim(temp), &PosErreur, 10);
		trim_effectue = 1;
	} else {
		resultat = strtod(valeur, &PosErreur);
	}
	// La commande precedente n'a pas renvoyer d'erreur
	if (*PosErreur == '\0') {
		if (resultat >= INT_MIN && resultat <= INT_MAX) {
			*variable = (int)resultat;
		} else {
			LOG_ERROR("La balise contient un chiffre, mais ce n'est pas un entier\n");
		}
	} else {
		LOG_ERROR("La balise ne contient pas de chiffre\n");
	}
	if (trim_effectue == 1) {
		free(temp);
	}
}
/* Lecture des valeurs des composantes d'une couleur :
 *
 * L'élément XML fourni doit contenir une balise <rvb /> (ou <rvba />) indiquant
 * les composantes rouge, vert, bleu (et alpha) dans des attributs (r, v, b et a)
 *
 * Les valeurs doivent être comprises entre 0 et 1 mais il est possible d'utiliser
 * la notation de 0 à 255 dans les champs XML : pour celà toute valeur supérieure
 * à 1 sera systématiquement divisée par 255.
 */
void lecture_rvb(const xmlNode *noeud, couleur_rvb *couleur) {
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (!xmlStrEqual(noeud->name, (xmlChar *)"rvb")) {
			continue;
		}
		strtod_securise(get_string_property(noeud, "r"), &couleur->rouge);
		strtod_securise(get_string_property(noeud, "v"), &couleur->vert);
		strtod_securise(get_string_property(noeud, "b"), &couleur->bleu);
		break;
	}
	if (couleur->rouge > 1) {
		couleur->rouge /= 255;
	}
	if (couleur->vert > 1) {
		couleur->vert /= 255;
	}
	if (couleur->bleu > 1) {
		couleur->bleu /= 255;
	}
}
void lecture_rvba(const xmlNode *noeud, couleur_rvba *couleur) {
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (!xmlStrEqual(noeud->name, (xmlChar *)"rvba")) {
			continue;
		}
		strtod_securise(get_string_property(noeud, "r"), &couleur->rouge);
		strtod_securise(get_string_property(noeud, "v"), &couleur->vert);
		strtod_securise(get_string_property(noeud, "b"), &couleur->bleu);
		strtod_securise(get_string_property(noeud, "a"), &couleur->alpha);
		break;
	}
	if (couleur->rouge > 1) {
		couleur->rouge /= 255;
	}
	if (couleur->vert > 1) {
		couleur->vert /= 255;
	}
	if (couleur->bleu > 1) {
		couleur->bleu /= 255;
	}
	if (couleur->alpha > 1) {
		couleur->alpha /= 255;
	}
}
/* Convertion d'une chaine, en couleur définie dans client_server.h
 * et utilisées en paramètre de LOG_TO_CONSOLE.
 * Par exemple : c_red1, c_yellow2, c_green1...
 */
void lecture_palette(const xmlNode *noeud, Uint8 *couleur) {
	char *label = trim(strdup((const char *)noeud->children->content));
	// Note : Les couleurs rouge ont été retirés,
	// pour qu'elles continuent a être reservées à la modération.
	if (!strcmp(label, "green1")) {
		*couleur = c_green1;
	} else if (!strcmp(label, "green2")) {
		*couleur = c_green2;
	} else if (!strcmp(label, "green3")) {
		*couleur = c_green3;
	} else if (!strcmp(label, "green4")) {
		*couleur = c_green4;
	} else if (!strcmp(label, "yellow1")) {
		*couleur = c_yellow1;
	} else if (!strcmp(label, "yellow2")) {
		*couleur = c_yellow2;
	} else if (!strcmp(label, "yellow3")) {
		*couleur = c_yellow3;
	} else if (!strcmp(label, "yellow4")) {
		*couleur = c_yellow4;
	} else if (!strcmp(label, "orange1")) {
		*couleur = c_orange1;
	} else if (!strcmp(label, "orange2")) {
		*couleur = c_orange2;
	} else if (!strcmp(label, "orange3")) {
		*couleur = c_orange3;
	} else if (!strcmp(label, "orange4")) {
		*couleur = c_orange4;
	} else if (!strcmp(label, "purple1")) {
		*couleur = c_purple1;
	} else if (!strcmp(label, "purple2")) {
		*couleur = c_purple2;
	} else if (!strcmp(label, "purple3")) {
		*couleur = c_purple3;
	} else if (!strcmp(label, "purple4")) {
		*couleur = c_purple4;
	} else if (!strcmp(label, "blue1")) {
		*couleur = c_blue1;
	} else if (!strcmp(label, "blue2")) {
		*couleur = c_blue2;
	} else if (!strcmp(label, "blue3")) {
		*couleur = c_blue3;
	} else if (!strcmp(label, "blue4")) {
		*couleur = c_blue4;
	} else if (!strcmp(label, "grey1")) {
		*couleur = c_grey1;
	} else if (!strcmp(label, "grey2")) {
		*couleur = c_grey2;
	} else if (!strcmp(label, "grey3")) {
		*couleur = c_grey3;
	} else if (!strcmp(label, "grey4")) {
		*couleur = c_grey4;
	} else if (!strcmp(label, "rose1")) {
		*couleur = c_rose1;
	} else {
		LOG_ERROR("theme : couleurs - palette '%s' inconnue!\n", label);
	}
	free(label);
}
/*******************************************************************************
 * Fonctions traitant chacune des sections disponibles dans le thème :
 *
 * - section_palette    (type="palette")
 * - section_couleurs   (type="couleurs")
 * - section_polices    (type="polices")
 * - section_quickitems (type="barre_rapide")
 * - section_infocombat (type="info_combat")
 * - section_hudicons   (type="barre_icones")
 */
void section_palette(const xmlNode *noeud) {
	int global = get_int_property(noeud, "global");
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"couleur")) {
			int numero = get_int_property(noeud, "type");
			if ((numero >= 0) && (numero < 30)) {
				couleur_rvb couleur = {0.0, 0.0, 0.0};
				lecture_rvb(noeud, &couleur);
				colors_list[numero].r1 = 255 * couleur.rouge;
				colors_list[numero].g1 = 255 * couleur.vert;
				colors_list[numero].b1 = 255 * couleur.bleu;
				if (global) {
					colors_list[numero].r2 = 192 * couleur.rouge;
					colors_list[numero].g2 = 192 * couleur.vert;
					colors_list[numero].b2 = 192 * couleur.bleu;
					colors_list[numero].r3 = 128 * couleur.rouge;
					colors_list[numero].g3 = 128 * couleur.vert;
					colors_list[numero].b3 = 128 * couleur.bleu;
					colors_list[numero].r4 = 64 * couleur.rouge;
					colors_list[numero].g4 = 64 * couleur.vert;
					colors_list[numero].b4 = 64 * couleur.bleu;
				}
				LOG_DEBUG("palette: (%02d) [%d:%d:%d] [%d:%d:%d] [%d:%d:%d] [%d:%d:%d]\n", numero, colors_list[numero].r1, colors_list[numero].g1, colors_list[numero].b1, colors_list[numero].r2, colors_list[numero].g2, colors_list[numero].b2, colors_list[numero].r3, colors_list[numero].g3, colors_list[numero].b3, colors_list[numero].r4, colors_list[numero].g4, colors_list[numero].b4);
			} else {
				LOG_ERROR("theme : Numéro de palette non valide : %i (accepté de 0 à 29)\n", numero);
			}
		} else {
			LOG_ERROR("theme : palette - balise '%s' inconnue !\n", noeud->name);
		}
	}
}
// -----------------------------------------------------------------------------
couleur_rvb couleur_titre = {0.8, 0.8, 0.8};
couleur_rvb couleur_nom_pnj_dialogue = {1.0, 1.0, 1.0};
couleur_rvb couleur_texte_aide = {0.0, 0.0, 0.0};
couleur_rvba couleur_fond_aide = {1.0, 1.0, 0.5, 1.0};
couleur_rvba couleur_fond_fenetre = {0.0, 0.0, 0.0, 0.4};
couleur_rvba couleur_bord_fenetre = {0.77, 0.57, 0.39, 0.00};
couleur_rvba couleur_ligne_fenetre = {0.77, 0.57, 0.39, 0.00};
couleur_rvba fr_quickitems_zerocolor = {0.80, 0.00, 0.00, 0.50};
couleur_rvba fr_quickitems_coolcolor = {0.14, 0.35, 0.82, 0.50};
Uint8 couleur_mp = c_orange1;
Uint8 couleur_canal_actif = c_grey1;
Uint8 couleur_canaux = c_grey2;
Uint8 couleur_dev_canal = c_purple1;
Uint8 couleur_coord_canal = c_purple2;
Uint8 couleur_mod_canal = c_blue1;
Uint8 couleur_mg_canal = c_blue3;
Uint8 couleur_ig_canal = c_blue2;
void section_couleurs(const xmlNode *noeud) {
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"couleur")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "titre_perso")) {
				lecture_rvb(noeud, &couleur_titre);
			} else if (!strcmp(prop, "nom_dialogue")) {
				lecture_rvb(noeud, &couleur_nom_pnj_dialogue);
			} else if (!strcmp(prop, "infobulle_texte")) {
				lecture_rvb(noeud, &couleur_texte_aide);
			} else if (!strcmp(prop, "infobulle_fond")) {
				lecture_rvba(noeud, &couleur_fond_aide);
			} else if (!strcmp(prop, "fond_fenetre")) {
				lecture_rvba(noeud, &couleur_fond_fenetre);
			} else if (!strcmp(prop, "bord_fenetre")) {
				lecture_rvba(noeud, &couleur_bord_fenetre);
			} else if (!strcmp(prop, "ligne_fenetre")) {
				lecture_rvba(noeud, &couleur_ligne_fenetre);
			} else if (!strcmp(prop, "item_cooldown")) {
				lecture_rvba(noeud, &fr_quickitems_coolcolor);
			} else if (!strcmp(prop, "item_zero_qte")) {
				lecture_rvba(noeud, &fr_quickitems_zerocolor);
			}
		} else if (xmlStrEqual(noeud->name, (xmlChar *)"palette")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "defaut")) {
				lecture_palette(noeud, &couleur_canaux);
			} else if (!strcmp(prop, "actif")) {
				lecture_palette(noeud, &couleur_canal_actif);
			} else if (!strcmp(prop, "mp")) {
				lecture_palette(noeud, &couleur_mp);
			} else if (!strcmp(prop, "mg")) {
				lecture_palette(noeud, &couleur_mg_canal);
			} else if (!strcmp(prop, "ig")) {
				lecture_palette(noeud, &couleur_ig_canal);
			} else if (!strcmp(prop, "mod")) {
				lecture_palette(noeud, &couleur_mod_canal);
			} else if (!strcmp(prop, "coord")) {
				lecture_palette(noeud, &couleur_coord_canal);
			} else if (!strcmp(prop, "dev")) {
				lecture_palette(noeud, &couleur_dev_canal);
			}
		} else {
			LOG_ERROR("theme : couleurs - balise '%s' inconnue !\n", noeud->name);
		}
	}
	LOG_DEBUG("couleurs MISC: [%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f]\n", couleur_titre.rouge, couleur_titre.vert, couleur_titre.bleu, couleur_nom_pnj_dialogue.rouge, couleur_nom_pnj_dialogue.vert, couleur_nom_pnj_dialogue.bleu);
	LOG_DEBUG("couleurs IHM: [%0.2f:%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f:%0.2f]\n", couleur_fond_fenetre.rouge, couleur_fond_fenetre.vert, couleur_fond_fenetre.bleu, couleur_fond_fenetre.alpha, couleur_bord_fenetre.rouge, couleur_bord_fenetre.vert, couleur_bord_fenetre.bleu, couleur_bord_fenetre.alpha, couleur_ligne_fenetre.rouge, couleur_ligne_fenetre.vert, couleur_ligne_fenetre.bleu, couleur_ligne_fenetre.alpha);
	LOG_DEBUG("couleurs QI: [%0.2f:%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f:%0.2f] \n", fr_quickitems_coolcolor.rouge, fr_quickitems_coolcolor.vert, fr_quickitems_coolcolor.bleu, fr_quickitems_coolcolor.alpha, fr_quickitems_zerocolor.rouge, fr_quickitems_zerocolor.vert, fr_quickitems_zerocolor.bleu, fr_quickitems_zerocolor.alpha);
	LOG_DEBUG("palettes: %d %d %d %d %d %d %d %d\n", couleur_canaux, couleur_canal_actif, couleur_mp, couleur_mg_canal, couleur_ig_canal, couleur_mod_canal, couleur_coord_canal, couleur_dev_canal);
}
// -----------------------------------------------------------------------------
int police_titre = 0;
int police_livre = 2;
int police_carte = 7;
int police_nom_pnj_dialogue = 0;
void section_polices(const xmlNode *noeud) {
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"fonte")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "titre_perso")) {
				strtol_securise((const char *)noeud->children->content, &police_titre);
			} else if (!strcmp(prop, "texte_livre")) {
				strtol_securise((const char *)noeud->children->content, &police_livre);
			} else if (!strcmp(prop, "marque_carte")) {
				strtol_securise((const char *)noeud->children->content, &police_carte);
			} else if (!strcmp(prop, "nom_dialogue")) {
				strtol_securise((const char *)noeud->children->content, &police_nom_pnj_dialogue);
			}
		} else {
			LOG_ERROR("theme : polices - balise '%s' inconnue !\n", noeud->name);
		}
	}
	LOG_DEBUG("polices: %d %d\n", police_titre, police_nom_pnj_dialogue);
}
// -----------------------------------------------------------------------------
int fr_quickitems_dim = 30; // dimension d'une case (largeur=hauteur, hors cadre)
int fr_quickitems_div = 4; // nombre de cases formant un groupe
int fr_quickitems_sep = 3; // taille du séparateur entre les groupes de cases
int quickspells_ico = 24; // taille des icones de la barre rapide
int quickspells_dim = 30; // dimension d'une case (largeur=hauteur, hors cadre)
int quickspells_div = 4; // nombre de cases formant un groupe
int quickspells_sep = 3; // taille du séparateur entre les groupes de cases
void section_quickitems(const xmlNode *noeud) {
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"valeur")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "sorts_icone")) {
				strtol_securise((const char *)noeud->children->content, &quickspells_ico);
				if (quickspells_ico < 12) {
					quickspells_ico = 12;
				} else if (quickspells_ico > 64) {
					quickspells_ico = 64;
				}
			} else if (!strcmp(prop, "sorts_case")) {
				strtol_securise((const char *)noeud->children->content, &quickspells_dim);
				if (quickspells_dim < 12) {
					quickspells_dim = 12;
				} else if (quickspells_dim > 64) {
					quickspells_dim = 64;
				}
			} else if (!strcmp(prop, "sorts_groupe")) {
				strtol_securise((const char *)noeud->children->content, &quickspells_div);
				if (quickspells_div < 1) {
					quickspells_div = QUICKSPELLS_MAXSIZE;
				}
			} else if (!strcmp(prop, "sorts_delta")) {
				strtol_securise((const char *)noeud->children->content, &quickspells_sep);
				if (quickspells_sep < -32) {
					quickspells_sep = -32;
				} else if (quickspells_sep > 32) {
					quickspells_sep = 32;
				}
			} else if (!strcmp(prop, "items_case")) {
				strtol_securise((const char *)noeud->children->content, &fr_quickitems_dim);
				if (fr_quickitems_dim < 12) {
					fr_quickitems_dim = 12;
				} else if (fr_quickitems_dim > 64) {
					fr_quickitems_dim = 64;
				}
			} else if (!strcmp(prop, "items_groupe")) {
				strtol_securise((const char *)noeud->children->content, &fr_quickitems_div);
				if (fr_quickitems_div < 1) {
					fr_quickitems_div = FR_QUICKITEMS_MAXSIZE;
				}
			} else if (!strcmp(prop, "items_delta")) {
				strtol_securise((const char *)noeud->children->content, &fr_quickitems_sep);
				if (fr_quickitems_sep < -32) {
					fr_quickitems_sep = -32;
				} else if (fr_quickitems_sep > 32) {
					fr_quickitems_sep = 32;
				}
			}
		} else {
			LOG_ERROR("theme : quickitems - balise '%s' inconnue !\n", noeud->name);
		}
	}
	LOG_DEBUG("quickspells: %d %d %d %d\n", quickspells_ico, quickspells_dim, quickspells_div, quickspells_sep);
	LOG_DEBUG("quickitems: %d %d %d\n", fr_quickitems_dim, fr_quickitems_div, fr_quickitems_sep);
}
// -----------------------------------------------------------------------------
int console_coup_critique = 1;
int console_chance_critique = 0;
int console_chance_esquive = 0;
int console_degat_feu = 1;
int console_degat_froid = 1;
int console_degat_magie = 1;
int console_degat_lumiere = 1;
int console_degat_poison = 0;
int flottant_coup_critique = 1;
int flottant_chance_critique = 0;
int flottant_chance_esquive = 0;
int flottant_degat_feu = 0;
int flottant_degat_froid = 0;
int flottant_degat_magie = 0;
int flottant_degat_lumiere = 0;
int flottant_degat_poison = 0;
couleur_rvb couleur_degat_feu = {1.0, 0.1, 0.1};
couleur_rvb couleur_degat_froid = {0.2, 0.2, 0.8};
couleur_rvb couleur_degat_magie = {0.9, 0.7, 0.3};
couleur_rvb couleur_degat_lumiere = {0.95, 0.33, 1.f};
couleur_rvb couleur_degat_poison = {0.0, 0.7, 0.0};
void section_infocombat(const xmlNode *noeud) {
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"couleur")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "degat_feu")) {
				lecture_rvb(noeud, &couleur_degat_feu);
			} else if (!strcmp(prop, "degat_froid")) {
				lecture_rvb(noeud, &couleur_degat_froid);
			} else if (!strcmp(prop, "degat_magie")) {
				lecture_rvb(noeud, &couleur_degat_magie);
			} else if (!strcmp(prop, "degat_lumiere")) {
				lecture_rvb(noeud, &couleur_degat_lumiere);
			} else if (!strcmp(prop, "degat_poison")) {
				lecture_rvb(noeud, &couleur_degat_poison);
			}
		} else if (xmlStrEqual(noeud->name, (xmlChar *)"console")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "coup_critique")) {
				console_coup_critique = get_bool_value(noeud);
			} else if (!strcmp(prop, "chance_critique")) {
				console_chance_critique = get_bool_value(noeud);
			} else if (!strcmp(prop, "chance_esquive")) {
				console_chance_esquive = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_feu")) {
				console_degat_feu = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_froid")) {
				console_degat_froid = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_magie")) {
				console_degat_magie = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_lumiere")) {
				console_degat_lumiere = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_poison")) {
				console_degat_poison = get_bool_value(noeud);
			}
		} else if (xmlStrEqual(noeud->name, (xmlChar *)"flottant")) {
			const char *prop = get_string_property(noeud, "type");
			if (!strcmp(prop, "coup_critique")) {
				flottant_coup_critique = get_bool_value(noeud);
			} else if (!strcmp(prop, "chance_critique")) {
				flottant_chance_critique = get_bool_value(noeud);
			} else if (!strcmp(prop, "chance_esquive")) {
				flottant_chance_esquive = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_feu")) {
				flottant_degat_feu = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_froid")) {
				flottant_degat_froid = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_magie")) {
				flottant_degat_magie = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_lumiere")) {
				flottant_degat_lumiere = get_bool_value(noeud);
			} else if (!strcmp(prop, "degat_poison")) {
				flottant_degat_poison = get_bool_value(noeud);
			}
		} else {
			LOG_ERROR("theme : infocombat - balise '%s' inconnue !\n", noeud->name);
		}
	}
	LOG_DEBUG("infocombat couleur : [%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f] [%0.2f:%0.2f:%0.2f]\n", couleur_degat_feu.rouge, couleur_degat_feu.vert, couleur_degat_feu.bleu, couleur_degat_froid.rouge, couleur_degat_froid.vert, couleur_degat_froid.bleu, couleur_degat_magie.rouge, couleur_degat_magie.vert, couleur_degat_magie.bleu, couleur_degat_lumiere.rouge, couleur_degat_lumiere.vert, couleur_degat_lumiere.bleu, couleur_degat_poison.rouge, couleur_degat_poison.vert, couleur_degat_poison.bleu);
	LOG_DEBUG("infocombat console : %d %d %d %d %d %d %d\n", console_coup_critique, console_chance_critique, console_degat_feu, console_degat_froid, console_degat_magie, console_degat_lumiere, console_degat_poison);
	LOG_DEBUG("infocombat flottant: %d %d %d %d %d %d %d\n", flottant_coup_critique, flottant_chance_critique, flottant_degat_feu, flottant_degat_froid, flottant_degat_magie, flottant_degat_lumiere, flottant_degat_poison);
}
// -----------------------------------------------------------------------------
int icon_posx_theme[MAX_ICONES] = {-1};
void section_hudicons(const xmlNode *noeud) {
	int icon_pos = 0;
	int i, icon_size = 32;
	for (i = 0; i < MAX_ICONES; i++) {
		icon_posx_theme[i] = -99;
	}
	i = get_int_property(noeud, "marge");
	if (i > 0) {
		icon_size += i;
	}
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"espace")) {
			i = get_int_property(noeud, "taille");
			icon_pos += (i < 0) ? 8 : i;
		} else if (xmlStrEqual(noeud->name, (xmlChar *)"icone")) {
			int icon_no = -1;
			const char *prop = get_string_property(noeud, "type");
			if (get_int_property(noeud, "masquer") == 1) {
				continue;
			}
			if ((++icon_no < MAX_ICONES) && !strcmp(prop, "marcher")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "asseoir")) {                     } else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "regarder")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "utiliser")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "utiliser_avec")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "echanger")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "attaquer")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "inventaire")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "sorts")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "fabrication")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "carte")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "notes")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "amis")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "stats")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "console")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "aide")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "options")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "musiques")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "quitter")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "obtenir")) {} else if ((++icon_no < MAX_ICONES) && !strcmp(prop, "fabriquer")) {} else {
				LOG_ERROR("theme : hudicons - type '%s' inconnu !\n", prop);
				continue;
			}
			icon_posx_theme[icon_no] = icon_pos;
			icon_pos += icon_size;
		} else {
			LOG_ERROR("theme : hudicons - balise '%s' inconnue !\n", noeud->name);
		}
	}
}
/*******************************************************************************
 * Utilisation d'un thème XML :
 *
 * Chargement du fichier XML correspondant au thème indiqué.
 * Parcours des éléments XML et affectation des valeurs trouvées aux variables du jeu
 */
// Chargement des éléments de personnalisation renseignés dans le thème XML
int recherche_sections(const xmlNode *noeud) {
	if (!xmlStrEqual(noeud->name, (xmlChar *)"Theme")) {
		return 0;
	}
	for (noeud = noeud->children; noeud; noeud = noeud->next) {
		if (noeud->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (xmlStrEqual(noeud->name, (xmlChar *)"section")) {
			const char *type = get_string_property(noeud, (char *)"type");
			if (!strcmp(type, "palette")) {
				section_palette(noeud);
			} else if (!strcmp(type, "couleurs")) {
				section_couleurs(noeud);
			} else if (!strcmp(type, "polices")) {
				section_polices(noeud);
			} else if (!strcmp(type, "barre_rapide")) {
				section_quickitems(noeud);
			} else if (!strcmp(type, "info_combat")) {
				section_infocombat(noeud);
			} else if (!strcmp(type, "barre_icones")) {
				section_hudicons(noeud);
			} else {
				LOG_ERROR("theme : section '%s' inconnue !\n", type);
			}
		} else {
			LOG_ERROR("theme : section attendue - Balise '%s' inconnue !\n", noeud->name);
		}
	}
	return 1;
}
// Ouverture du fichier XML correspondant au thème spécifié pour lancer son chargement */
void init_theme(char *titre) {
	xmlDoc *doc;
	int i;
	for (i = 0; i < liste_themes_nb; i++) {
		if (strcmp(liste_themes[i].titre, titre) == 0) {
			LOG_INFO("theme : chargement du fichier xml (%s) pour '%s'\n", liste_themes[i].fichier, titre);
			doc = xmlReadFile(liste_themes[i].fichier, NULL, 0);
			if (doc != NULL) {
				recherche_sections(xmlDocGetRootElement(doc));
			} else {
				LOG_ERROR("theme : echec à l'ouverture du fichier xml (%s) !\n", liste_themes[i].fichier);
			}
			xmlFreeDoc(doc);
			return;
		}
	}
	LOG_ERROR("theme : le thème spécifié (%s) n'a pas été trouvé dans la liste des thèmes\n", titre);
}
/*******************************************************************************
 * Gestion de la liste des thèmes disponibles :
 *
 * Les thèmes XML sont recherchés dans tous les dossiers pouvant en contenir.
 * Chaque fichier trouvés est ouvert pour valider qu'il s'agit bien d'un thème.
 * Le champs "détail" est lu à cette occasion pour fournir une description.
 */
// Ouverture du fichier XML d'un thème pour récupérer sa description et valider son enregistrement
int chargement_theme_detail(int n) {
	xmlNode *root;
	xmlDoc *doc;
	int ok = 1;
	doc = xmlReadFile(liste_themes[n].fichier, NULL, 0);
	if (doc == NULL) {
		LOG_ERROR("%s : %s \"%s\"\n", reg_error_str, liste_themes[n].fichier, cant_open_file);
		return 0;
	}
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		LOG_ERROR("theme : impossible de traiter le fichier du theme '%s'", liste_themes[n].fichier);
		ok = 0;
	} else if (!xmlStrEqual(root->name, (xmlChar *)"Theme")) {
		LOG_ERROR("theme : balise inconnue '%s' ('Theme' attendu).", root->name);
		ok = 0;
	} else {
		strncpy(liste_themes[n].detail, get_string_property(root, (char *)"detail"), sizeof(liste_themes[n].detail));
	}
	xmlFreeDoc(doc);
	return ok;
}
// Parcours le dossier spécifié à la recherche de fichiers XML de thèmes à ajouter dans la liste
int chargement_liste_themes(char *dossier_themes) {
	DIR *dp;
	struct dirent *ep;
	int i, deja;
	LOG_INFO("theme : recherche dans le dossier '%s'", dossier_themes);
	dp = opendir(dossier_themes);
	if (dp == NULL) {
		return 0;
	}
	while ((ep = readdir(dp)) != NULL) {
		/* on ne garde que les fichiers contenant un 'titre' entre 1 et 20 caractères */
		if ((strlen(ep->d_name) <= 4) || (strlen(ep->d_name) > 24)) {
			continue;
		}
		/* et qui se terminent par le suffixe '.xml' */
		if (strcmp(ep->d_name + strlen(ep->d_name) - 4, ".xml") != 0) {
			continue;
		}
		/* on détermine le titre du thème en retirant le suffixe du nom du fichier */
		safe_snprintf(liste_themes[liste_themes_nb].titre, strlen(ep->d_name) - 3, "%s", ep->d_name);
		/* le xml io-wrapper se charge de chercher dans les différents dossiers de base possible */
		safe_snprintf(liste_themes[liste_themes_nb].fichier, sizeof(liste_themes[liste_themes_nb].fichier), "themes/%s", ep->d_name);
		/* vérification qu'un thème avec ce nom n'est pas déjà dans la liste */
		deja = 0;
		for (i = 0; i < liste_themes_nb && !deja; i++) {
			if (strcmp(liste_themes[i].titre, liste_themes[liste_themes_nb].titre) == 0) {
				deja = 1;
			}
		}
		if (deja) {
			continue;
		}
		/* vérification que la liste des thèmes n'est pas pleine ! */
		if (liste_themes_nb >= LISTE_THEMES_MAX) {
			LOG_ERROR("theme : nombre de thèmes excessif (max %i) : fichier '%s' non chargé !\n", LISTE_THEMES_MAX, ep->d_name);
			continue;
		}
		/* validation de l'enregistrement du thème trouvé dans la liste */
		if (chargement_theme_detail(liste_themes_nb)) {
			liste_themes_nb++;
		}
	}
	closedir(dp);
	return 1;
}
// Initialise la liste des thèmes disponibles depuis les dossiers updates, datadir puis local
void init_liste_themes() {
	char dossier_themes[256];
	// on remet la liste à zéro (en cas de rechargement de la liste)
	liste_themes_nb = 0;
	// on cherche d'abord la présence de thèmes dans le dossier perso (conf locale)
	safe_snprintf(dossier_themes, sizeof(dossier_themes), "%s/themes/", get_path_config());
	chargement_liste_themes(dossier_themes);
	// on cherche ensuite la présence de thèmes dans le dossier des mises à jour auto
	safe_snprintf(dossier_themes, sizeof(dossier_themes), "%s/themes/", get_path_updates());
	chargement_liste_themes(dossier_themes);
	// on récupère ensuite les thèmes disponible dans le dossier de données du jeu
	safe_snprintf(dossier_themes, sizeof(dossier_themes), "%s/themes/", datadir);
	chargement_liste_themes(dossier_themes);
	// éventuellement on regarde dans un dossier de thèmes à partir du dossier courant (normalement datadir aussi)
	safe_snprintf(dossier_themes, sizeof(dossier_themes), "%s/themes/", ".");
	chargement_liste_themes(dossier_themes);
}
/*******************************************************************************
 * Commandes de gestion in-game du thème actif
 *
 * Ces fonctions correspondent à des commandes consoles
 * permettant d'afficher la liste des thèmes dispo
 * et de sélectionner le thème à utiliser.
 */
// Affiche à l'écran la liste des thèmes en indiquant en surbrillance le thème actif
int command_liste_themes() {
	char str[200];
	int i;
	init_liste_themes();
	LOG_TO_CONSOLE(c_green2, "Liste des thèmes disponibles :");
	for (i = 0; i < liste_themes_nb; i++) {
		safe_snprintf(str, sizeof(str), "-%-12s : %s", liste_themes[i].titre, liste_themes[i].detail);
		LOG_TO_CONSOLE(strcmp(titre_theme, liste_themes[i].titre) ? c_yellow1 : c_yellow2, str);
	}
	return 1;
}
// Sélectionne le nouveau thème actif qui sera chargé et mémorisé dans le fichier de conf
int command_change_theme(char *text) {
	char str[200];
	int i;
	init_liste_themes();
	for (; isspace(*text); text++) {}
	for (i = 0; i < liste_themes_nb; i++) {
		if (strcmp(text, liste_themes[i].titre) == 0) {
			safe_snprintf(titre_theme, sizeof(titre_theme), "%s", text);
			set_var_unsaved("titre_theme", OPT_STRING_INI); // forcer la sauvegarde dans le fichier ini
			init_theme(titre_theme);
			LOG_TO_CONSOLE(c_green1, "Le thème actif a été modifié et appliqué.");
			// redimensionnement des barres rapides
			resize_fr_quickitems();
			resize_quickspells(-1);
			// rechargement de la barre d'icone du hud
			// Ackak suppression vu la fusion US.
			return 1;
		}
	}
	safe_snprintf(str, sizeof(str), "Ce thème ne figure pas dans la liste ! (%s)", text);
	LOG_TO_CONSOLE(c_red1, str);
	return 1;
}
