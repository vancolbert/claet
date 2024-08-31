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


#ifdef FR_VERSION
extern float book_zoom; /*!< zoom factor for book text */
extern int book_reload;
#endif //FR_VERSION

#ifdef FR_VERSION
extern int livre_ouvert; //Le numéro du livre actuellement ouvert
#else //FR_VERSION
/*! The ID of the book currently open*/
extern int book_opened;
#endif //FR_VERSION

#ifdef FR_DEBUG_LIVRES
/*!
 * \ingroup	livre
 * \brief	Permet de tester un livre en local
 *
 * 		Ouvre un livre en local sans rien demander au serveur
 *
 * \param	données             les données venant du serveur
 * \param	longueur_donnees    la longueur des données
 *
 * \callgraph
 */
void lire_livre_local (const char *donnees, int longueur_donnees);
#else //FR_DEBUG_LIVRES
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
void read_local_book (const char * data, int len);
#endif //FR_DEBUG_LIVRES

#ifdef FR_VERSION
/*!
 * \ingroup livre
 * \brief   Initialise les livres locaux non gérés par le serveur
 *
 *      Initialise les livres locaux, qui ne sont pas gérés par le serveur
 *
 * \callgraph
 */
void init_livres();
#else //FR_VERSION
/*!
 * \ingroup	books_window
 * \brief	Reads some books that will not be asked for server-side
 *
 * 		Reads a few local books that we will not ask the server about...
 */
void init_books();
#endif //FR_VERSION

/*!
 * \ingroup	books_window
 * \brief	Frees the memory allocated for books
 *
 * 		Frees the memory allocated for books
 */
void free_books();

#ifdef FR_VERSION
/*!
 * \ingroup	livre
 * \brief	Permet de savoir si le livre est local ou vient du serveur
 *
 * 		Le traitement permet de savoir si le livre est local ou serveur. S'il est local, il faudra ouvrir le fichier xml autrement il faudra traiter les données reçues qui seront elles aussi en xml
 *
 * \param	données             les données venant du serveur
 * \param	longueur_donnees    la longueur des données
 *
 * \callgraph
 */
void lire_livre_reseau (const char * donnees, int longueur_donnees);
#else //FR_VERSION
/*!
 * \ingroup	network_books
 * \brief	Selects the parser for the book send from the server
 *
 * 		When the server sends a book to the client the first byte will be used to specify the parser that's going to be used - whether the book is local and uses xml or if it's server-side and uses the network data parser.
 *
 * \param	data The network data
 * \param	len The length of the data
 *
 * \callgraph
 */
void read_network_book (const char * data, int len);
#endif //FR_VERSION

#ifdef FR_VERSION
/*!
 * \ingroup	livre
 * \brief	Ouvre le livre désigné
 *
 *      Ouvre le livre dont le numéro est donné en argument
 *
 * \param	num_livre   le numéro du livre qu'il faut ouvrir
 *
 * \callgraph
 */
void ouvre_livre(int num_livre);
#else //FR_VERSION
/*!
 * \ingroup	network_books
 * \brief	Opens the book with the given ID
 *
 * 		Opens the book with the given ID - if the book isnt found it will send a SEND_BOOK followed by the ID to the server.
 *
 * \param	id The book ID
 *
 * \callgraph
 */
void open_book(int id);
#endif //FR_VERSION

#ifdef FR_VERSION
/*!
 * \ingroup	livre
 * \brief	Ferme le livre désigné
 *
 * 		Ferme le livre dont le numéro est donné en argument
 *
 * \param	num_livre   le numéro du livre qu'il faut fermer
 *
 * \callgraph
 */
int ferme_livre(int num_livre);
#else //FR_VERSION
/*!
 * \ingroup	books_window
 * \brief	Closes the book window with the given id
 *
 * 		Closes the book window with the given id, if it's opened
 *
 * \param	book_id The unique book ID
 */
void close_book(int book_id);
#endif //FR_VERSION

#ifdef FR_VERSION
void change_page (int num_livre, int nb_pages, int positif);

int affiche_image(char* texte);
#endif //FR_VERSION

#ifdef __cplusplus
} // extern "C"
#endif

#endif
