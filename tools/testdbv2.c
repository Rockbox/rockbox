#include <stdlib.h>
#include <stdio.h>

#define ARTISTLEN        28
#define ALBUMLEN         24
#define SONGLEN          52
#define GENRELEN         12
#define FILELEN          96
#define SONGARRAYLEN     14
#define ALBUMARRAYLEN    1


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

FILE *fp;

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

int main() {
   fp=fopen("rockbox.id3db","r");
   int *p,i,temp,temp2,temp3,temp4;
   if(fp<0) return -1;
   fread(&header,sizeof(header),1,fp);
   p=&header;
   for(i=0;i<17;i++) {
	   *p=BE32(*p);
	   p++;
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
     printf("\n\nShow artist(1)/album(2)/song(3)/file(4) ? ");
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
     }
   } while(1);
   printf("\n");
   fseek(fp,SongEntry.artist,SEEK_SET);
   fread(&ArtistEntry,sizeof(struct ArtistEntry),1,fp);
   printf("Offset: 0x%x\nArtist: %s\n",SongEntry.artist,ArtistEntry.name);
   for(i=0;i<header.albumarraylen;i++) {
	   ArtistEntry.album[i]=BE32(ArtistEntry.album[i]);
	   printf("Album %d: 0x%x\n",i,ArtistEntry.album[i]);
   }
   printf("\n");
   fseek(fp,SongEntry.file,SEEK_SET);
   fread(&FileEntry,sizeof(struct FileEntry),1,fp);
   FileEntry.hash=BE32(FileEntry.hash);
   FileEntry.tagoffset=BE32(FileEntry.tagoffset);
   FileEntry.rundboffset=BE32(FileEntry.rundboffset);
   printf("Offset: 0x%x\nFilename: %s\nHash: 0x%x\nTag: 0x%x\nRunDB: 0x%x\n",
		   SongEntry.file,FileEntry.name,FileEntry.hash,
		   FileEntry.tagoffset, FileEntry.rundboffset);
   
   /*fseek(fp,AlbumEntry.artist,SEEK_SET);
   fread(&ArtistEntry,sizeof(struct ArtistEntry),1,fp);
   printf("Offset: 0x%x\nArtist: %s\n",AlbumEntry.artist,ArtistEntry.name);
   for(i=0;i<header.albumarraylen;i++) {
      ArtistEntry.album[i]=BE32(ArtistEntry.album[i]);
      printf("Album %d: 0x%x\n",i,ArtistEntry.album[i]);
   } */  
   fclose(fp);
}
