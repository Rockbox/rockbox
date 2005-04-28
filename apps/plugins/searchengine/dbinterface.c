#include "searchengine.h"
#include "dbinterface.h"

#undef SONGENTRY_SIZE
#undef FILEENTRY_SIZE
#undef ALBUMENTRY_SIZE
#undef ARTISTENTRY_SIZE
#undef FILERECORD2OFFSET

#define SONGENTRY_SIZE    (rb->tagdbheader->songlen+12+rb->tagdbheader->genrelen+4)
#define FILEENTRY_SIZE    (rb->tagdbheader->filelen+12)
#define ALBUMENTRY_SIZE   (rb->tagdbheader->albumlen+4+rb->tagdbheader->songarraylen*4)
#define ARTISTENTRY_SIZE  (rb->tagdbheader->artistlen+rb->tagdbheader->albumarraylen*4)

#define FILERECORD2OFFSET(_x_) (rb->tagdbheader->filestart + _x_ * FILEENTRY_SIZE)

struct entry *currententry;

static struct entry *entryarray;

int database_init() {
    char *p;
    unsigned int i;
    // allocate room for all entries
    entryarray=(struct entry *)my_malloc(sizeof(struct entry)*rb->tagdbheader->filecount);
    p=(char *)entryarray;
    // zero all entries.
    for(i=0;i<sizeof(struct entry)*rb->tagdbheader->filecount;i++) 
	    *(p++)=0;
    if(*rb->tagdb_initialized!=1) {
        if(!rb->tagdb_init()) {
            // failed loading db
            return -1;
        }
    }
    return 0;
}

void loadentry(int filerecord) {
    if(entryarray[filerecord].loadedfiledata==0) {
        rb->lseek(*rb->tagdb_fd,FILERECORD2OFFSET(filerecord),SEEK_SET);
        entryarray[filerecord].filename=(char *)my_malloc(rb->tagdbheader->filelen);
	rb->read(*rb->tagdb_fd,entryarray[filerecord].filename,rb->tagdbheader->filelen);
	rb->read(*rb->tagdb_fd,&entryarray[filerecord].hash,4);
	rb->read(*rb->tagdb_fd,&entryarray[filerecord].songentry,4);
	rb->read(*rb->tagdb_fd,&entryarray[filerecord].rundbentry,4);
	entryarray[filerecord].loadedfiledata=1;
    }
    currententry=&entryarray[filerecord];
}

void loadsongdata() {
    if(currententry->loadedsongdata || 
        !currententry->loadedfiledata) 
        return;
    currententry->title=(char *)my_malloc(rb->tagdbheader->songlen);
    currententry->genre=(char *)my_malloc(rb->tagdbheader->genrelen);
    rb->lseek(*rb->tagdb_fd,currententry->songentry,SEEK_SET);
    rb->read(*rb->tagdb_fd,currententry->title,rb->tagdbheader->songlen);
    rb->read(*rb->tagdb_fd,&currententry->artistoffset,4);
    rb->read(*rb->tagdb_fd,&currententry->albumoffset,4);
    rb->lseek(*rb->tagdb_fd,4,SEEK_CUR);
    rb->read(*rb->tagdb_fd,currententry->genre,rb->tagdbheader->genrelen);
    rb->read(*rb->tagdb_fd,&currententry->bitrate,2);
    rb->read(*rb->tagdb_fd,&currententry->year,2);
    currententry->loadedsongdata=1;
}

void loadrundbdata() {
	// we don't do this yet.
    currententry->loadedrundbdata=1;
}

void loadartistname() {
   /* memory optimization possible, only malloc for an album name once, then
    * write that pointer to the entrys using it.
   */
   currententry->artistname=(char *)my_malloc(rb->tagdbheader->artistlen);
   rb->lseek(*rb->tagdb_fd,currententry->artistoffset,SEEK_SET);
   rb->read(*rb->tagdb_fd,currententry->artistname,rb->tagdbheader->artistlen);
   currententry->loadedartistname=1;
}

void loadalbumname() {
   /* see the note at loadartistname */
   currententry->albumname=(char *)my_malloc(rb->tagdbheader->albumlen);
   rb->lseek(*rb->tagdb_fd,currententry->albumoffset,SEEK_SET);
   rb->read(*rb->tagdb_fd,currententry->albumname,rb->tagdbheader->albumlen);
   currententry->loadedalbumname=1;
}

char *getfilename(int entry) {
   if(entryarray[entry].loadedfiledata==0)
	   return "error O.o;;;";
   else
	   return entryarray[entry].filename;
}
