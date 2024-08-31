#ifndef __THEME_H__
#define __THEME_H__
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
	char titre[20]; // titre = valeur de l'option = nom du fichier xml sans le suffixe
	char fichier[256]; // chemin complet vers le fichier xml du thème (updates ou datadir)
	char detail[200]; // description du thème (fournie dans le fichier xml du thème)
} theme;
#define LISTE_THEMES_MAX 20 // nombre maximum de thèmes dans la liste
extern theme liste_themes[LISTE_THEMES_MAX]; // liste des thèmes trouvés (updates + datadir)
extern int liste_themes_nb; // nombre d'éléments dans la liste des thèmes
extern char titre_theme[20]; // titre du thème à utiliser
/*! \{ */
/*! Structure pour les couleurs RVB*/
typedef struct {
	float rouge;
	float vert;
	float bleu;
} couleur_rvb;
/* @} */
/*! \{ */
/*! Structure pour les couleurs RVB avec le canal alpha*/
typedef struct {
	float rouge;
	float vert;
	float bleu;
	float alpha;
} couleur_rvba;
/* @} */
/*!
 * @{ */
extern Uint8 couleur_mp; /*!< Couleur utilisée pour les MP */
extern Uint8 couleur_canal_actif; /*!< Couleur du canal actif */
extern Uint8 couleur_canaux; /*!< Couleur des canaux non-actifs */
extern Uint8 couleur_dev_canal; /*!< Couleur du canal #dev */
extern Uint8 couleur_coord_canal; /*!< Couleur du canal #coord */
extern Uint8 couleur_mod_canal; /*!< Couleur du canal #mod */
extern Uint8 couleur_mg_canal; /*!< Couleur du canal #mg */
extern Uint8 couleur_ig_canal; /*!< Couleur du canal #ig */
extern couleur_rvb couleur_titre; /*!< Couleur utilisée pour l'affichage du titre au-dessus des personnages */
extern couleur_rvb couleur_nom_pnj_dialogue; /*!< Couleur utilisée pour le nom du PNJ lors du dialogue */
extern couleur_rvb couleur_texte_aide; /*!< Couleur utilisée pour le texte (et le cadre) des bulles d'aide */
extern couleur_rvba couleur_fond_aide; /*!< Couleur utilisée pour le fond des bulles d'aide */
extern couleur_rvba couleur_fond_fenetre; /*!< Couleur utilisée pour le fond des fenêtres */
extern couleur_rvba couleur_bord_fenetre; /*!< Couleur utilisée pour le bord des fenêtres */
extern couleur_rvba couleur_ligne_fenetre; /*!< Couleur utilisée pour les lignes des fenêtres */
extern couleur_rvba fr_quickitems_zerocolor; /*!< Couleur de la case lorsque le type d'objet est a zero */
extern couleur_rvba fr_quickitems_coolcolor; /*!< Couleur de la progression du cooldown sur la case */
extern int police_titre; /*!< Police utilisée pour l'affichage du titre au-dessus des personnes */
extern int police_livre; /*!< Police utilisée pour le texte des livres */
extern int police_carte; /*!< Police utilisée pour les marques des cartes */
extern int police_nom_pnj_dialogue; /*!< Police utilisée pour le nom du PNJ lors du dialogue */
extern int quickspells_ico; /*!< Taille des icones de la barre rapide */
extern int quickspells_dim; /*!< Taille d'une case de la barre rapide (hors cadre) */
extern int quickspells_div; /*!< Nombre de case pour avoir un trait de separation */
extern int quickspells_sep; /*!< Taille de la separation dans le barre rapide */
extern int fr_quickitems_dim; /*!< Taille d'une case de la barre rapide (hors cadre) */
extern int fr_quickitems_div; /*!< Nombre de case pour avoir un trait de separation */
extern int fr_quickitems_sep; /*!< Taille de la separation dans le barre rapide */
extern int console_coup_critique; /*!< Affiche ou non les coups critiques dans la console */
extern int console_chance_critique; /*!< Affiche ou non les chances critiques dans la console */
extern int console_chance_esquive; /*!< Affiche ou non les chances esquive critique dans la console */
extern int console_degat_feu; /*!< Affiche ou non les dégâts de feu dans la console */
extern int console_degat_froid; /*!< Affiche ou non les dégâts de froid dans la console */
extern int console_degat_magie; /*!< Affiche ou non les dégâts de magie dans la console */
extern int console_degat_lumiere; /*!< Affiche ou non les dégâts de lumière dans la console */
extern int console_degat_poison; /*!< Affiche ou non les dégâts de poison dans la console */
extern int flottant_coup_critique; /*!< Affiche ou non les coups critiques avec des messages flottants */
extern int flottant_chance_critique; /*!< Affiche ou non les chances critiques avec des messages flottants */
extern int flottant_chance_esquive; /*!< Affiche ou non les esquives critiques avec des messages flottants */
extern int flottant_degat_feu; /*!< Affiche ou non les dégâts de feu avec des messages flottants */
extern int flottant_degat_froid; /*!< Affiche ou non les dégâts de froid avec des messages flottants */
extern int flottant_degat_magie; /*!< Affiche ou non les dégâts de magie avec des messages flottants */
extern int flottant_degat_lumiere; /*!< Affiche ou non les dégâts de lumière avec des messages flottants */
extern int flottant_degat_poison; /*!< Affiche ou non les dégâts de poison avec des messages flottants */
extern couleur_rvb couleur_degat_feu; /*!< Couleur pour les messages flottants des dégâts de feu */
extern couleur_rvb couleur_degat_froid; /*!< Couleur pour les messages flottants des dégâts de froid */
extern couleur_rvb couleur_degat_magie; /*!< Couleur pour les messages flottants des dégâts de magie */
extern couleur_rvb couleur_degat_lumiere; /*!< Couleur pour les messages flottants des dégâts de lumière */
extern couleur_rvb couleur_degat_poison; /*!< Couleur pour les messages flottants des dégâts de poison */
#define MAX_ICONES 30 /*!< Nombre maximum d'icones dans la barre */
extern int icon_posx_theme[MAX_ICONES]; /*!< Tableau pour l'ordre d'affichage des icones */
/* @} */
/*!
 *  \ingroup theme
 *  \brief  Convertit une chaine de caractere en reel (float)
 *
 *      Convertion d'un chaine de caractere en reel (float)
 *
 *  \param valeur   Le contenu de la balise que l'on souhaite traite
 *  \param variable Adresse de la variable de retour
 *  \param xmlNode  Le noeud qu'il faut traites
 *
 *  \callgraph
 */
void strtod_securise(const char *valeur, float *variable);
/*!
 *  \ingroup theme
 *  \brief  Convertit une chaine de caractere en entier (int)
 *
 *      Convertion d'un chaine de caractere en entier (int)
 *
 *  \param valeur   Le contenu de la balise que l'on souhaite traite
 *  \param variable Adresse de la variable de retour
 *  \param xmlNode  Le noeud qu'il faut traites
 *
 *  \callgraph
 */
void strtol_securise(const char *valeur, int *variable);
/*!
 *  \ingroup theme
 *  \brief  Traite le fichier xml d'un thème
 *
 *      Vérifie la présence du fichier xml du thèmes
 *      Traitement du fichier pour charger les valeurs des variables définies
 *
 *  \param nom_fichier le nom du fichier xml du thème à charger
 *
 *  \callgraph
 */
void init_theme(char *titre);
/*!
 *  \ingroup theme
 *  \brief  Traite la liste de thèmes du fichier themes.xml
 *
 *      Vérifie la présence du fichier themes.xml
 *      Parcours le fichier pour initialiser la liste des thèmes disponibles
 *
 *  \callgraph
 */
void init_liste_themes();
/*!
 *  \ingroup theme
 *  \brief Affiche la liste des thèmes
 *
 *		Affiche à l'écrna la liste des thèmes en indiquant en surbrillance le thème actif
 *
 *	\callgraph
 */
int command_liste_themes();
/*!
 *  \ingroup theme
 *  \brief Sélectionne le nouveau thème
 *
 * 	Sélectionne le nouveau thème actif qui sera chargé et mémorisé dans le fichier de conf
 *
 *	\callgraph
 */
int command_change_theme(char *text);
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif // __THEME_H__
