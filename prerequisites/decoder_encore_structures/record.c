#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#include <string.h>
#include <arpa/inet.h>

int record_init(struct record *r)
{
    r->type = 0;
    r->has_footer = 0;
    r->length = 0;
    
    r->data = NULL;
    
    return 0;
}

void record_free(struct record *r)
{
    if(r->data != NULL)
        free(r->data);
}

int record_get_type(const struct record *r)
{
    return r->type;
}

void record_set_type(struct record *r, int type)
{
    r->type = type;
}

int record_get_length(const struct record *r)
{
    return r->length;
}

int record_set_payload(struct record *r, const char * buf, int n)
{
    char* data = (char*)malloc(sizeof(char)*n);
    if(!data)
        return -1;
    
    memcpy(data, buf, n);
    
    r->length = n;
    if(r->data)
        free(r->data);
    r->data = data;

    return 0;
}

int record_get_payload(const struct record *r, char *buf, int n)
{
    int value;
    if(r->length < n){
        value = r->length;
    }
    else{
        value = n;
    }
    
    memcpy(buf, r->data, value);
    
    return value;
}

int record_has_footer(const struct record *r)
{
    return r->has_footer;
}

void record_delete_footer(struct record *r)
{
    r->has_footer = 0;
    r->UUID = 0;
}

void record_set_uuid(struct record *r, unsigned int uuid)
{
    if(!r->has_footer){
        r->has_footer = 1;
    }
    r->UUID = uuid;
}

unsigned int record_get_uuid(const struct record *r)
{
    return r->UUID;
}

int record_write(const struct record *r, FILE *f)
{   
    uint16_t length = r->length;
    length = htons(length);
    
    int sum = 0;
    sum += sizeof(uint16_t)*fwrite(r, sizeof(uint16_t), 1, f);
    sum += sizeof(uint16_t)*fwrite(&length, sizeof(uint16_t), 1, f);
    
    if(r->length > 0){
        sum += sizeof(char)*r->length*fwrite(r->data, r->length*sizeof(char), 1, f);
    }
    if(r->has_footer){
        uint32_t uuid = r->UUID;
        sum += sizeof(uint32_t)*fwrite(&uuid, sizeof(uint32_t), 1, f);
    }
    return sum;
}

int record_read(struct record *r, FILE *f)
{
    int sum = 0;
    sum += sizeof(uint16_t)*fread(r, sizeof(uint16_t), 1, f);

    uint16_t length;
    sum += sizeof(uint16_t)*fread(&length, sizeof(uint16_t), 1, f);
    length = ntohs(length);
    r->length = length;

    if(r->length){
        char* a = (char*)malloc(sizeof(char)*(length+1));
        if(!a)
            return -1;
        sum += sizeof(char)*length*fread(a, sizeof(char)*length, 1, f);
        r->data = a;
    }
    else{
        r->data = NULL;
    }

    if(r->has_footer){
        uint32_t foo;
        sum += sizeof(uint32_t)*fread(&foo, sizeof(uint32_t), 1, f);
        r->UUID = foo;
    }
    else{
        r->UUID = 0;
    }
    return sum;
}

int main(int argc, char* argv[]){
	
	system("rm test.dms");
	system("touch test.dms");
	struct record* r = (struct record*)malloc(sizeof(struct record));
	record_init(r);
	char* str = "abcdef0123456789bbcc";
	r->type = 23;
	r->has_footer = 0;
	r->length = 10;

	record_set_payload(r, str, strlen(str)+1);
	printf("%s\n", r->data);
	record_set_uuid(r, 1234);
	FILE *f = fopen("output.dms", "r+");
	struct record lector;
	record_read(&lector, f);
	

}




