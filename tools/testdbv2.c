#include <stdlib.h>
#include <stdio.h>

#define ARTISTLEN        60
#define ALBUMLEN         64
#define SONGLEN          120
#define GENRELEN         28
#define FILELEN          176
#define SONGARRAYLEN     715
#define ALBUMARRAYLEN    186


#define BE32(_x_) (((_x_ & 0xff000000) >> 24) | \
		                   ((_x_ & 0x00ff0000) >> 8) | \
		                   ((_x_ & 0x0000ff00) << 8) | \
		                   ((_x_ & 0x000000ff) << 24))
#define BE16(_x_)  ( ((_x_&0xff00) >> 8) | ((_x_&0xff)<<8))

struct header {
	int version;
	int artiststart;
	int albumstart;
	int songstart;
	int filestart;
	int artistcount;
	int albumcount;
	int songcount;
	int filecount;
	int artistlen;
	int albumlen;
	int songlen;
	int genrelen;
	int filelen;
	int songarraylen;
	int albumarraylen;
	int rundbdirty;
} header;

struct FileEntry {
   char name[FILELEN];
   int hash;
   int tagoffset;
   int rundboffset;
} FileEntry;

struct SongEntry {
   char name[SONGLEN];
   int artist;
   int album;
   int file;
   char genre[GENRELEN];
   short bitrate;
   short year;
} SongEntry;

struct ArtistEntry {
   char name[ARTISTLEN];
   int album[ALBUMARRAYLEN];
} ArtistEntry;

struct AlbumEntry {
   char name[ALBUMLEN];
   int artist;
   int song[SONGARRAYLEN];
} AlbumEntry;

struct RundbHeader {
   int version;
   int entrycount;
} RundbHeader;

struct RundbEntry {
   int file;
   int hash;
   short rating;
   short voladj;
   int playcount;
   int lastplayed;
} RundbEntry;

FILE *fp,*fp2;

void showsong(int offset) {
   fseek(fp,offset,SEEK_SET);
   fread(&SongEntry,sizeof(struct SongEntry),1,fp);
   SongEntry.artist=BE32(SongEntry.artist);
   SongEntry.album=BE32(SongEntry.album);
   SongEntry.file=BE32(SongEntry.file);
   SongEntry.year=BE16(SongEntry.year);
   SongEntry.bitrate=BE16(SongEntry.bitrate);
   printf("Offset: 0x%x\nName: %s\nArtist: 0x%x\nAlbum: 0x%x\nFile: 0x%x\nGenre: %s\nBitrate: %d\nYear: %d\n\n",offset,SongEntry.name,SongEntry.artist,SongEntry.album,SongEntry.file,SongEntry.genre,SongEntry.bitrate,SongEntry.year);
}

void showalbum(int offset) {
   int i;
   fseek(fp,offset,SEEK_SET);
   fread(&AlbumEntry,sizeof(struct AlbumEntry),1,fp);
   AlbumEntry.artist=BE32(AlbumEntry.artist);
   printf("Offset: 0x%x\nAlbum: %s\nArtist: 0x%x\n",offset,AlbumEntry.name,AlbumEntry.artist);
   for(i=0;i<header.songarraylen;i++) {
           AlbumEntry.song[i]=BE32(AlbumEntry.song[i]);
           printf("Song %d: 0x%x\n",i,AlbumEntry.song[i]);
   }
}

void showfile(int offset) {
   fseek(fp,offset,SEEK_SET);
   fread(&FileEntry,sizeof(struct FileEntry),1,fp);
   FileEntry.hash=BE32(FileEntry.hash);
   FileEntry.tagoffset=BE32(FileEntry.tagoffset);
   FileEntry.rundboffset=BE32(FileEntry.rundboffset);
   printf("Offset: 0x%x\nFilename: %s\nHash: 0x%x\nTag: 0x%x\nRunDB: 0x%x\n",
                   offset,FileEntry.name,FileEntry.hash,
                   FileEntry.tagoffset, FileEntry.rundboffset);
}

void showartist(int offset) {
   int i;
   fseek(fp,offset,SEEK_SET);
   fread(&ArtistEntry,sizeof(struct ArtistEntry),1,fp);
   printf("Offset: 0x%x\nArtist: %s\n",offset,ArtistEntry.name);
   for(i=0;i<header.albumarraylen;i++) {
           ArtistEntry.album[i]=BE32(ArtistEntry.album[i]);
           printf("Album %d: 0x%x\n",i,ArtistEntry.album[i]);
   }
}

void showrundb(int offset) {
    fseek(fp2,offset,SEEK_SET);
    fread(&RundbEntry,sizeof(struct RundbEntry),1,fp2);
    RundbEntry.file=BE32(RundbEntry.file);
    RundbEntry.hash=BE32(RundbEntry.hash);
    RundbEntry.playcount=BE32(RundbEntry.playcount);
    RundbEntry.lastplayed=BE32(RundbEntry.lastplayed);
    RundbEntry.rating=BE16(RundbEntry.rating);
    RundbEntry.voladj=BE16(RundbEntry.voladj);
    printf("Offset: 0x%x\nFileEntry: 0x%x\nHash: 0x%x\nRating: %d\nVoladj: 0x%x\n",offset,RundbEntry.file,RundbEntry.hash,RundbEntry.rating,RundbEntry.voladj);
    printf("Playcount: 0x%x\nLastplayed: %d\n",RundbEntry.playcount,RundbEntry.lastplayed);
}

int main() {
   fp=fopen("rockbox.tagdb","r");
   fp2=fopen("rockbox.rundb","r");
   int *p,i,temp,temp2,temp3,temp4;
   if(fp<0) return -1;
   fread(&header,sizeof(header),1,fp);
   p=&header;
   for(i=0;i<17;i++) {
	   *p=BE32(*p);
	   p++;
   }
   if(fp2>=0) {
       fread(&RundbHeader,sizeof(RundbHeader),1,fp2);
       p=&RundbHeader;
       for(i=0;i<2;i++) {
           *p=BE32(*p);
           p++;
       }
   }
   printf("db version    : 0x%x\n",header.version&0xFF);
   printf("Artist start  : 0x%x\n",header.artiststart);
   printf("Album start   : 0x%x\n",header.albumstart);
   printf("Song start    : 0x%x\n",header.songstart);
   printf("File start    : 0x%x\n",header.filestart);
   printf("Artist count  : %d\n",header.artistcount);
   printf("Album count   : %d\n",header.albumcount);
   printf("Song count    : %d\n",header.songcount);
   printf("File count    : %d\n",header.filecount);
   printf("Artist len    : %d\n",header.artistlen);
   printf("Album len     : %d\n",header.albumlen);
   printf("Song len      : %d\n",header.songlen);
   printf("Genre len     : %d\n",header.genrelen);
   printf("File len      : %d\n",header.filelen);
   printf("Songarraylen  : %d\n",header.songarraylen);
   printf("Albumarraylen : %d\n",header.albumarraylen);
   printf("Rundb dirty   : %d\n",header.rundbdirty);
   if(fp2>=0) {
       printf("Rundb version : 0x%x\n",RundbHeader.version&0xFF);
       printf("Rundb entrys  : %d\n",RundbHeader.entrycount);
   }
   if( sizeof(struct SongEntry)!=(header.songlen+header.genrelen+16)) {
		printf("Song Entry Size mismatch.. update the code to correct size.\n");
              return;
   }
   if(sizeof(struct AlbumEntry)!=(header.albumlen+4+header.songarraylen*4)) {
printf("Album Entry Size mismatch.. update the code to correct size.\n");
              return;
   }
   if(sizeof(struct ArtistEntry)!=(header.artistlen+header.albumarraylen*4)) {
printf("Artist Entry Size mismatch.. update the code to correct size.\n");
              return;
   }
   if(sizeof(struct FileEntry)!=(header.filelen+12)) {
	   printf("File Entry Size mismatch.. update the code to correct size.\n");
	   return;
   }

   do {
     printf("\n\nShow artist(1)/album(2)/song(3)/file(4)");
     if(fp2>=0) printf("/rundb(5)");
     printf(" ? ");
     fflush(stdout);
     temp=temp2=temp3=0;
     scanf("%d",&temp);
     printf("Record (-1 for offset) ? ");
     fflush(stdout);
     scanf("%d",&temp2);
     if(temp2==-1) {
	  printf("Offset ? 0x");
	  fflush(stdout);
	  scanf("%x",&temp3);
     }
     switch(temp) {
	     case 1:
		     if(temp2==-1)
			     showartist(temp3);
		     else
			     showartist(header.artiststart + 
					     temp2*sizeof(struct ArtistEntry));
		     break;
	     case 2:
                     if(temp2==-1)
                             showalbum(temp3);
                     else
                             showalbum(header.albumstart +
                                             temp2*sizeof(struct AlbumEntry));
		     break;
	     case 3:
                     if(temp2==-1)
                             showsong(temp3);
                     else
                             showsong(header.songstart +
                                             temp2*sizeof(struct SongEntry));
		     break;
	     case 4:
                     if(temp2==-1)
                             showfile(temp3);
                     else
                             showfile(header.filestart +
                                             temp2*sizeof(struct FileEntry));
		     break;
         case 5:    if(temp2==-1)
                             showrundb(temp3);
                    else
                             showrundb(8+temp2*sizeof(struct RundbEntry));
             break;
             default:
		     return;
		     break;
     }
   } while(1);
   fclose(fp);
}
