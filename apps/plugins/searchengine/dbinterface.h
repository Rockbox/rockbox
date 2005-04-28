struct entry {
   int loadedfiledata,
       loadedsongdata,
       loadedrundbdata,
       loadedalbumname,
       loadedartistname;
   char *filename;
   int hash;
   int songentry;
   int rundbentry;
   short year;
   short bitrate;
   int rating;
   int playcount;
   char *title;
   char *genre;
   int artistoffset;
   int albumoffset;
   char *artistname;
   char *albumname;
};

extern struct entry *currententry;
extern struct entry *entryarray;

int database_init(void);
void loadentry(int filerecord);
void loadsongdata(void);
void loadrundbdata(void);
void loadartistname(void);
void loadalbumname(void);
char *getfilename(int entry);
