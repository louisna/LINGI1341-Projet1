#include "packet_implement.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h> // ntohs && htons
#include <zlib.h>


/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
    uint8_t window:5;
    uint8_t truncated:1;
    uint8_t type:2;
    uint8_t seqnum;
    uint16_t length;
    uint32_t timestamp;
    uint32_t crc1;
    char* payload;
    uint32_t crc2;
};

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
    pkt_t* new = (pkt_t*)calloc(1, sizeof(pkt_t));
    if(!new)
        return NULL;

    return new;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt!=NULL){
        if(pkt->length>0 || pkt->payload!=NULL)
            free(pkt->payload);

        free(pkt);
    }
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    if(data == NULL)
        return E_UNCONSISTENT;

    int l = len;
    l++;

    /* copie le champ header sans le timestamp */
    memcpy(pkt, data, sizeof(uint32_t));

    if(pkt->type < 1 || pkt->type > 3){
        fprintf(stderr, "Incorrect packet type\n");
        pkt_del(pkt);
        return E_TYPE;
    }

    if(pkt->truncated){
        if(pkt->type != PTYPE_DATA){
            fprintf(stderr, "The packet was truncated but it is not a data packet\n");
            pkt_del(pkt);
            return E_TYPE;
        }
    }

    /* window and sequence number alreay written */

    /* putting the length in host from network by order */
    pkt->length = ntohs(pkt->length);
    if(pkt->length > MAX_PAYLOAD_SIZE){
        fprintf(stderr, "Maximum legnth reached\n");
        return E_LENGTH;
    }

    /* timestamp */
    memcpy(&(pkt->timestamp), &data[4], sizeof(uint32_t));

    /* crc1 */
    memcpy(&(pkt->crc1), &data[8], sizeof(uint32_t));
    pkt->crc1 = ntohl(pkt->crc1);

    uint32_t crc_load = pkt->crc1;

    uint8_t flag = pkt->truncated;
    if(pkt_set_tr(pkt, 0) != PKT_OK){
        return E_TR;
    }

    pkt->length = htons(pkt->length);

    //printf("decode: %d\n", pkt->length);

    //uLong crc1 = crc32(0L, Z_NULL, 0);
    uint32_t crc = (uint32_t) crc32(0, (Bytef*) pkt, sizeof(uint64_t));
    pkt->truncated = flag;

    if(crc != crc_load){
        fprintf(stderr, "CRC1 invalid, error\n");
        return E_CRC;
    }

    pkt->length = ntohs(pkt->length);

    int taille = pkt->length;

    if(taille != 0){
        char* payload = (char*)malloc(sizeof(char)*taille);
        if(!payload){
            fprintf(stderr, "Error malloc\n");
            return E_NOMEM;
        }

        memcpy(payload, data+12, taille*sizeof(char));

        uint32_t crc2_2;

        memcpy(&crc2_2, data+12+taille, 4);
        crc2_2 = ntohl(crc2_2);
        if(crc32(0, (Bytef*) data+12, taille) != crc2_2){
            fprintf(stderr, "crc2 mauvais\n");
            return E_CRC;
        }

        pkt->payload = payload;
        pkt->crc2 = crc2_2;
    }

    return 0;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    int current = 0;

    if(buf == NULL || *len == 0){
        return E_NOMEM;
    }

    if(pkt == NULL){
        return 0;
    }

    int total = sizeof(uint32_t)*3;
    if(pkt->crc2){
        total += sizeof(uint32_t);
    }
    if(pkt->length > 0){
        total += pkt->length;
    }

    if(*len < (size_t) total){
        fprintf(stderr, "Error, not enough memory\n");
        return E_NOMEM;
    }

    memcpy(buf, pkt, sizeof(uint16_t));
    current += sizeof(uint16_t);

    uint16_t length = htons(pkt->length);

    memcpy(buf+current, &length, sizeof(uint16_t));
    current += sizeof(uint16_t);

    memcpy(buf+current, &(pkt->timestamp), sizeof(uint32_t));
    current += sizeof(uint32_t);

    //char buft[8];
    //memcpy(buft, buf, 8);

    pkt_t temp22;
    memcpy(&temp22, pkt, 8);
    pkt_t* temp = &temp22;

    //temp = (pkt_t*)buft;

    temp->length = htons(pkt->length);

    if(pkt_set_tr(temp, 0) != PKT_OK){
        return E_TR;
    }

    //uLong crc = crc32(0L, Z_NULL, 0);
    uint32_t crc1 = crc32(0,(Bytef*) temp, current);
    // printf("encode: %d, length de pkt: %d\n", temp->length, pkt->length);
    crc1 = htonl(crc1);

    memcpy(buf+current, &crc1, sizeof(uint32_t));
    current += sizeof(uint32_t);

    if(pkt->length){
        memcpy(buf+current, pkt->payload, sizeof(char)*pkt->length);
        current += sizeof(char)*pkt->length;

        uLong crc2 = crc32(0, (Bytef*) pkt->payload, pkt->length);
        crc2 = htonl(crc2);
        memcpy(buf+current, &crc2, sizeof(uint32_t));
        current += sizeof(uint32_t);
    }

    *len = current;

    return 0;
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
    return pkt->truncated;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp(const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2(const pkt_t* pkt)
{
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    if(pkt->length)
        return pkt->payload;
    return NULL;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if(type > 3 || type < 0)
		return E_TYPE;
    pkt->type = type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	if(tr > 1 || tr < 0)
		return E_TR;
    pkt->truncated = tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if(window > MAX_WINDOW_SIZE)
		return E_WINDOW;
    pkt->window = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	if(seqnum > 255 || seqnum < 0)
		return E_SEQNUM;
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if(length > MAX_PAYLOAD_SIZE || length < 0)
		return E_LENGTH;
    pkt->length = length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2 = crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{

    if(length > MAX_PAYLOAD_SIZE){
        return E_LENGTH;
    }

    if(pkt->length > 0){
        free(pkt->payload);
        pkt->length = 0;
    }

    if(data==NULL){
        pkt->length = 0;
        pkt->payload = NULL;
        return 0;
    }

    char* p = (char*)malloc(sizeof(char)*length);
    if(!p){
        return E_NOMEM;
    }

    memcpy(p, data, length);

    pkt->payload = p;
    pkt->length = length;

    return PKT_OK;
}

void print_data(pkt_t* pkt){
	printf("type: %d\n truncated: %d\n seqnum: %d\n window: %d\n length: %d\n crc1: %d\n payload: %s\n",
		pkt->type, pkt->truncated, pkt->seqnum, pkt->window, pkt->length, pkt->crc1, pkt->payload);
}




