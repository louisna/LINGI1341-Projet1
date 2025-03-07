#ifndef __RECORD_H_
#define __RECORD_H_

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#include <string.h>

struct record {
	uint16_t type : 15;
	uint16_t has_footer : 1;
	uint16_t length;
    char* data;
	uint32_t UUID;
};


/**
* Initialise un enregistrement
* @pre: r!= NULL
* @post: record_get_type(r) == 0 && record_get_length(r) == 0
*		 && record_has_footer(r) == 0
* @return: 1 en cas d'erreur, 0 sinon
*/
int record_init(struct record *r);

/**
* LibÃ¨re les ressources consommÃ©es par un enregistrement
* @pre: r!= NULL
*/
void record_free(struct record *r);

/**
* Renvoie le type d'un enregistrement
* @pre: r != NULL
*/
int record_get_type(const struct record *r);

/**
* DÃ©finit le type d'un enregistrement
* @pre: r != NULL
* @post: record_get_type(r) == (type & 0x7FFF)
*/
void record_set_type(struct record *r, int type);

/**
* Renvoie la taille du payload de l'enregistrement
* @pre: r!= NULL
*/
int record_get_length(const struct record *r);

/**
* DÃ©finit le payload de l'enregistrement, en copiant n octets
* du buffer. Si le buffer est NULL (ou de taille 0), supprime
* le payload
* @pre: r != NULL
* @post: record_get_length(r) == (n & 0xFFFF)
		 && record_get_payload(<buf2>, n) == (n & 0xFFFF)
*        && memcmp(buf, <buf2>, (n & 0xFFFF)) == 0
* @return: -1 en cas d'erreur, 0 sinon
*/
int record_set_payload(struct record *r,
			 const char * buf, int n);

/**
* Copie jusqu'Ã  n octets du payload dans un buffer
* prÃ©-allouÃ© de taille n
* @pre: r != NULL && buf != NULL && n > 0
* @return: n', le nombre d'octets copiÃ©s dans le buffer
* @post: n' <= n && n' <= record_get_length(r)
*/
int record_get_payload(const struct record *r,
			 char *buf, int n);

/**
* Teste si l'enregistrement possÃ¨de un footer
* @pre: r != NULL
* @return: 1 si l'enregistrement a un footer, 0 sinon
*/
int record_has_footer(const struct record *r);

/**
* Supprime le footer d'un enregistrement
* @pre: r != NULL
* @post: record_has_footer(r) == 0
*/
void record_delete_footer(struct record *r);

/**
* DÃ©finit l'uuid d'un enregistrement
* @pre: r != NULL
* @post: record_has_footer(r) &&
	  record_get_uuid(r, &<uuid2>) => uuid2 == uuid
*/
void record_set_uuid(struct record *r, unsigned int uuid);

/**
* Extrait l'uuid d'un enregistrement
* @pre: r != NULL
* @post: (record_has_footer(r) && uuid != 0) ||
*        (!record_has_footer(r) && uuid == 0)
*/
unsigned int record_get_uuid(const struct record *r);

/**
* Ecrit un enregistrement dans un fichier
* @pre: r != NULL && f != NULL
* @return: n', le nombre d'octets Ã©crits dans le fichier.
	   -1 en cas d'erreur
*/
int record_write(const struct record *r, FILE *f);

/**
* Lit un enregistrement depuis un fichier
* @pre: r != NULL && f != NULL
* @return: n', le nombre d'octets luts dans le fichier.
	   -1 en cas d'erreur
*/
int record_read(struct record *r, FILE *f);


#endif /* __RECORD_H_ */