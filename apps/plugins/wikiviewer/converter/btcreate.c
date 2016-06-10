/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <string.h>
#include "../shared/utf8_aux.h"
#include "btree.h"

#define KEYLEN_MAX 200
#define DEBUG 0

/**A key is a int_fast_16_t that holds the size in char's of the rest of the
   key, and then the rest of the key*/
static unsigned char *getkey(void* key)
{
    return (unsigned char *)(key+sizeof(uint_fast16_t));
}

static char compare(void *key1,void *key2)
{
#if DEBUG == 1
    printf("CMP:%s,%s:%d:%d,%d\n", getkey(key1), getkey(key2),
                                   utf8strcnmp(getkey(key1), getkey(key2),
                                               *(uint_fast16_t*)key1,
                                               *(uint_fast16_t*)key2, false),
                                   *(uint_fast16_t*)key1,
                                   *(uint_fast16_t*)key2);
#endif

    return utf8strcnmp(getkey(key1), getkey(key2),
                       *(uint_fast16_t*)key1, *(uint_fast16_t*)key2, false);
}

static void int16le(uint16_t val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
}

static void int32le(uint32_t val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

static unsigned short bnode_write_len(void *tree, bt_node *node) /* depends on
                                                                    key/data
                                                                    structure */
{
    (void)tree;
    /*Static node length:
       1:uint8_t (flags:leaf) 2:uint16_t (nr_active) 3*/
    unsigned int i;
    uint16_t bnodelen=3;
    for(i=0; i < node->nr_active; i++)
    {
        /*Static key length:
           8:uint64_t (datapointer) 4:uint32_t (childpointer) 2:uint16_t
           (keylength) 14
         */
        bnodelen += (*(uint_fast16_t*)node->key_vals[i]->key);
        bnodelen += 14;
    }
    if(!node->leaf)
        bnodelen += 4;

    return bnodelen;
}

static void bnode_write(FILE *file,void *tree,bt_node *node,uint32_t *chldptrs) /*
                                                                                   depends
                                                                                   on
                                                                                   key/data
                                                                                   structure
                                                                                 */
{
    unsigned char bnode_write_buffer[((btree*)tree)->node_write_len(tree,node)];
    memset(bnode_write_buffer,111,((btree*)tree)->node_write_len(tree,node));
    unsigned short bfcntr=0,i=0;
    bnode_write_buffer[bfcntr]=node->leaf;
    bfcntr++;
    int16le(node->nr_active,&bnode_write_buffer[bfcntr]);
    bfcntr+=2;
    for(i=0; i < node->nr_active; i++)
    {
        /*Why do I have to write these (dataptrs) inverse of how they were
           readed?*/
        int32le(((uint32_t)(*((uint32_t*)node->key_vals[i]->val))),&bnode_write_buffer[bfcntr]);
        bfcntr+=4;
        int32le(((uint32_t)(*((uint32_t*)(node->key_vals[i]->val+sizeof(uint32_t))))),&bnode_write_buffer[bfcntr]);
        bfcntr+=4;
        int32le(chldptrs[i],&bnode_write_buffer[bfcntr]);
        bfcntr+=4;
        int16le((*(uint_fast16_t*)node->key_vals[i]->key),&bnode_write_buffer[bfcntr]);
        bfcntr+=2;
        memcpy(&bnode_write_buffer[bfcntr],(node->key_vals[i]->key+sizeof(uint_fast16_t)),(*(uint_fast16_t*)node->key_vals[i]->key));
        bfcntr+=(*(uint_fast16_t*)node->key_vals[i]->key);
    }
    if(!node->leaf)
    {
        int32le(chldptrs[node->nr_active],&bnode_write_buffer[bfcntr]);
        bfcntr+=4;
    }

    fwrite(bnode_write_buffer,sizeof(char),bfcntr,file); /* is it correct with
                                                            sizeof */
}

static unsigned int keysize(void * key)
{
    return sizeof(uint_fast16_t)+(sizeof(char)*(*(uint_fast16_t*)key));
}

static unsigned int datasize(void * data)
{
    (void)data;
    return sizeof(uint32_t)*2;
}

static void *mallockey(unsigned char* str,uint_fast16_t length)
{
    void *ret = malloc(sizeof(uint_fast16_t)+(sizeof(unsigned char)*length));
    void *rtm=(ret+sizeof(uint_fast16_t));
    memcpy(rtm,str,length);
    *((uint_fast16_t*)ret)=length;
    return ret;
}

static bt_key_val* makekv(char* key,uint_fast16_t keylen,uint32_t data1,uint32_t data2)
{
    bt_key_val * kv;
    kv = (bt_key_val*)malloc(sizeof(bt_key_val));
    kv->key = mallockey((unsigned char*)key,keylen);
    kv->val = malloc(sizeof(uint32_t)*2);
    *(uint32_t *)kv->val = data1;
    *(uint32_t *)(kv->val+sizeof(uint32_t)) = data2;
    return kv;
}

int main(int argc,char * argv[])
{
    if(argc<4)
    {
        printf("Usage: <in-list> <redirect-list> <out-tree>\n");
        return 0;
    }

    btree * tree;
    bt_key_val * kv;
    void *key;
    bt_key_val * kvr;
    tree = btree_create(30);
    tree->key_size = keysize;
    tree->data_size = datasize;
    tree->compare = compare;
    tree->node_write_len=bnode_write_len;
    tree->node_write=bnode_write;
    char finlne[KEYLEN_MAX+1],tinlne[KEYLEN_MAX+1];
    char elmhdr[12];
    uint_fast16_t fkeylen=0,tkeylen=0;
    FILE *fd=fopen(argv[1],"r");
    while(fread(elmhdr,sizeof(uint8_t),12,fd)==12)
    {
        if(*((uint32_t*)&elmhdr[8])==0)
        {
            printf("Skipping empty\n");
            continue;
        }

        fkeylen=*((uint32_t*)&elmhdr[8])<KEYLEN_MAX ? *((uint32_t*)&elmhdr[8]) : KEYLEN_MAX;
        fread(finlne,sizeof(char),fkeylen,fd);
        if((*(uint32_t*)&elmhdr[8])-fkeylen>0)
            fseek(fd,(*((uint32_t*)&elmhdr[8]))-fkeylen,SEEK_CUR);

        finlne[fkeylen]=0;
        if(strlen(finlne)!=(fkeylen))
        {
            printf("Bad keylen: %s\n",finlne);
            continue;
        }

        kv=makekv(finlne,fkeylen,*((uint32_t*)&elmhdr),*((uint32_t*)&elmhdr[4]));
        btree_insert_key(tree,kv);
        kv=0;
    }
    printf("SCRLD\n");
    fclose(fd);
    fd=NULL;
    if(strlen(argv[2])!=0)
    {
        FILE *rfd=fopen(argv[2],"rb");
        while(fread(elmhdr,sizeof(uint8_t),8,rfd)==8)
        {
            if(*((uint32_t*)&elmhdr[0])==0||*((uint32_t*)&elmhdr[4])==0)
            {
                printf("Skipping empty\n");
                continue;
            }

            fkeylen=*((uint32_t*)&elmhdr[0])<KEYLEN_MAX ? *((uint32_t*)&elmhdr[0]) : KEYLEN_MAX;
            tkeylen=*((uint32_t*)&elmhdr[4])<KEYLEN_MAX ? *((uint32_t*)&elmhdr[4]) : KEYLEN_MAX;
            fread(finlne,sizeof(char),fkeylen,rfd);
            if((*(uint32_t*)&elmhdr[0])-fkeylen>0)
            {
                printf("MXLADJf %s,%d,skip %d\n",finlne,*(uint32_t*)&elmhdr[0],(*((uint32_t*)&elmhdr[0]))-fkeylen);
                fseek(rfd,(*((uint32_t*)&elmhdr[0]))-fkeylen,SEEK_CUR);
            }

            fread(tinlne,sizeof(char),tkeylen,rfd);
            if((*(uint32_t*)&elmhdr[4])-tkeylen>0)
            {
                printf("MXLADJt %s,%d,skip %d\n",tinlne,*(uint32_t*)&elmhdr[4],(*((uint32_t*)&elmhdr[4]))-tkeylen);
                fseek(rfd,(*((uint32_t*)&elmhdr[4]))-tkeylen,SEEK_CUR);
            }

            finlne[fkeylen]=0;
            tinlne[tkeylen]=0;
            if(strlen(finlne)!=(fkeylen)||strlen(tinlne)!=(tkeylen))
            {
                printf("Bad keylen: %s,%s,%d=%d,%d=%d\n",finlne,tinlne,strlen(finlne),(fkeylen),strlen(tinlne),(tkeylen));
                continue;
            }

            key=mallockey((unsigned char*) tinlne,tkeylen);
            if((kvr=btree_search(tree,key))!=NULL)
            {
                kv=makekv(finlne,fkeylen,(*((uint32_t*)kvr->val)),(*((uint32_t*)(kvr->val+sizeof(uint32_t)))));
                btree_insert_key(tree,kv);
            }

            free(key);
        }
        fclose(rfd);
        rfd=NULL;
    }

    FILE *ofd=fopen(argv[3],"w");
    btree_write(ofd,tree,tree->root);
    fclose(ofd);
    ofd=NULL;

    return 0;
}
