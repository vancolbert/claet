/*!
 * \file
 * \ingroup misc
 * \brief Used for the implementation of books in EL
 */
#ifndef __BOOKS_H__
#define __BOOKS_H__
#ifdef __cplusplus
extern "C" {
#endif
/*! The book window*/
extern int book_win;
/*! The paper window*/
extern int paper_win;
extern float book_zoom; /*!< zoom factor for book text */
extern int book_reload;
extern int livre_ouvert; // Le num�ro du livre actuellement ouvert
/*!
 * \ingroup	network_books
 * \brief	Opens a local book.
 *
 * 		Opens a local book - if it resides in memory already it will not be reloaded, but just redisplayed.
 *
 * \param	data The data from the server
 * \param	len The length of the data
 *
 * \callgraph
 */
void read_local_book(const char *data, int len);
/*!
 * \ingroup livre
 * \brief   Initialise les livres locaux non g�r�s par le serveur
 *
 *      Initialise les livres locaux, qui ne sont pas g�r�s par le serveur
 *
 * \callgraph
 */
void init_livres();
/*!
 * \ingroup	books_window
 * \brief	Frees the memory allocated for books
 *
 * 		Frees the memory allocated for books
 */
void free_books();
/*!
 * \ingroup	livre
 * \brief	Permet de savoir si le livre est local ou vient du serveur
 *
 * 		Le traitement permet de savoir si le livre est local ou serveur. S'il est local, il faudra ouvrir le fichier xml autrement il faudra traiter les donn�es re�ues qui seront elles aussi en xml
 *
 * \param	donn�es             les donn�es venant du serveur
 * \param	longueur_donnees    la longueur des donn�es
 *
 * \callgraph
 */
void lire_livre_reseau(const char *donnees, int longueur_donnees);
/*!
 * \ingroup	livre
 * \brief	Ouvre le livre d�sign�
 *
 *      Ouvre le livre dont le num�ro est donn� en argument
 *
 * \param	num_livre   le num�ro du livre qu'il faut ouvrir
 *
 * \callgraph
 */
void ouvre_livre(int num_livre);
/*!
 * \ingroup	livre
 * \brief	Ferme le livre d�sign�
 *
 * 		Ferme le livre dont le num�ro est donn� en argument
 *
 * \param	num_livre   le num�ro du livre qu'il faut fermer
 *
 * \callgraph
 */
int ferme_livre(int num_livre);
void change_page(int num_livre, int nb_pages, int positif);
int affiche_image(char *texte);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
