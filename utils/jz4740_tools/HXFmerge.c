/*
Made by Maurus Cuelenaere
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>

#define VERSION "0.2"

static unsigned char* int2le(unsigned int val)
{
    static unsigned char addr[4];
    addr[0] = val & 0xff;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
    return addr;
}

static unsigned int le2int(unsigned char* buf)
{
   unsigned int res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

#ifdef _WIN32
 #define PATH_SEPARATOR "\\"
#else
 #define PATH_SEPARATOR "/"
#endif

#ifndef _WIN32

#define MIN(a, b) (a > b ? b : a)
static char* replace(char* str)
{
    char tmp[255];
    memcpy(tmp, str, MIN(strlen(str), 255);
    char *ptr = tmp;
    while(*ptr != 0)
    {
        if(*ptr == 0x2F) /* /*/
            *ptr = 0x5C; /* \ */
        ptr++;
    }
    return tmp;
}
#endif

static bool is_dir(const char* name1, const char* name2)
{
    char *name;
    DIR *directory;
    name = (char*)malloc(strlen(name1)+strlen(name2)+1);
    strcpy(name, name1);
    strcat(name, name2);
    directory = opendir(name);
    free(name);
    if(directory)
    {
        closedir(directory);
        return true;
    }
    else
        return false;
}

unsigned int _filesize(FILE* fd)
{
    unsigned int tmp, oldpos;
    oldpos = ftell(fd);
    fseek(fd, 0, SEEK_END);
    tmp = ftell(fd);
    fseek(fd, oldpos, SEEK_SET);
    return tmp;
}
#define WRITE(x, len) if(fwrite(x, len, 1, outfile) != 1) \
                      { \
                          closedir(indir_handle); \
                          if(filesize > 0) \
                            free(buffer); \
                          fprintf(stderr, "[ERR]  Error writing to file\n"); \
                          return; \
                      }
static void merge_hxf(const char* indir, FILE* outfile, const char* add)
{
    DIR *indir_handle;
    struct dirent *dirs;
    char dir[255];
    strcpy(dir, indir);
    strcat(dir, add);
    
    if((indir_handle = opendir(dir)) == NULL)
    {
        fprintf(stderr, "[ERR]  Error opening dir %s\n", indir);
        return;
    }
    
    while((dirs = readdir(indir_handle)) != NULL)
    {
        if(strcmp(dirs->d_name, "..") != 0 &&
           strcmp(dirs->d_name, ".") != 0)
        {
            fprintf(stderr, "[INFO] %s\%s\n", add, dirs->d_name);
            if(is_dir(dir, dirs->d_name))
            {
                char dir2[255];
                strcpy(dir2, add);
                strcat(dir2, dirs->d_name);
                strcat(dir2, PATH_SEPARATOR);
                merge_hxf(indir, outfile, dir2);
            }
            else
            {
                FILE *filehandle;
                unsigned char *buffer;
                char file[255];
                unsigned int filesize;
                strcpy(file, dir);
                strcat(file, dirs->d_name);
            	if((filehandle = fopen(file, "rb")) == NULL)
                {
            		fprintf(stderr, "[ERR]  Cannot open %s\n", file);
                    closedir(indir_handle);
            		return;
            	}
                filesize = _filesize(filehandle);
                if(filesize > 0)
                {
                    buffer = (unsigned char*)malloc(filesize);
                    if(buffer == NULL)
                    {
                        fclose(filehandle);
                        closedir(indir_handle);
                        fprintf(stderr, "[ERR]  Cannot allocate memory\n");
                        return;
                    }
                    if(fread(buffer, filesize, 1, filehandle) != 1)
                    {
                        fclose(filehandle);
                        closedir(indir_handle);
                        free(buffer);
                        fprintf(stderr, "[ERR]  Cannot read from %s%s%s\n", add, PATH_SEPARATOR, dirs->d_name);
                        return;
                    }
                }
                fclose(filehandle);
                
                if(strlen(add)>0)
                {
                    WRITE(int2le(dirs->d_namlen+strlen(add)), 4);
#ifndef _WIN32
                    WRITE(replace(add), strlen(add)-1);
#else
                    WRITE(add, strlen(add)-1);
#endif
                    WRITE(PATH_SEPARATOR, 1);
                    WRITE(dirs->d_name, dirs->d_namlen);
                }
                else
                {
                    WRITE(int2le(dirs->d_namlen), 4);
                    WRITE(dirs->d_name, dirs->d_namlen);
                }
                WRITE(int2le(filesize), 4);
                if(filesize>0)
                {
                    WRITE(buffer, filesize);
                    free(buffer);
                }
            }
        }
    }
    closedir(indir_handle);
}

static void print_usage(void)
{
#ifdef _WIN32
    fprintf(stderr, "Usage: hxfmerge.exe [INPUT_DIR] [FW]\n\n");
    fprintf(stderr, "Example: hxfmerge.exe VX747_extracted\\ VX747.HXF\n\n");
#else
    fprintf(stderr, "Usage: HXFmerge [INPUT_DIR] [FW]\n\n");
    fprintf(stderr, "Example: HXFmerge VX747_extracted/ VX747.HXF\n\n");
#endif
}

static unsigned int checksum(FILE *file)
{
    int oldpos = ftell(file);
    unsigned int ret, i, filesize = _filesize(file)-0x40;
    unsigned char *buf;
    
    buf = (unsigned char*)malloc(filesize);
    
    if(buf == NULL)
    {
        fseek(file, oldpos, SEEK_SET);
        fprintf(stderr, "[ERR] Error while allocating memory\n");
        return 0;
    }
    
    fseek(file, 0x40, SEEK_SET);
    if(fread(buf, filesize, 1, file) != 1)
    {
        free(buf);
        fseek(file, oldpos, SEEK_SET);
        fprintf(stderr, "[ERR] Error while reading from file\n");
        return 0;
    }
    
    fprintf(stderr, "[INFO] Computing checksum...");
    
    for(i = 0; i < filesize; i+=4)
        ret += le2int(&buf[i]);
    
    free(buf);
    fseek(file, oldpos, SEEK_SET);
    
    fprintf(stderr, " Done!\n");
    return ret;
}

int main(int argc, char *argv[])
{
    FILE *outfile;
    
    fprintf(stderr, "HXFmerge v" VERSION " - (C) 2008 Maurus Cuelenaere\n");
    fprintf(stderr, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    
    if(argc != 3)
    {
        print_usage();
        return 1;
    }

#ifdef _WIN32
    if(strcmp((char*)(argv[1]+strlen(argv[1])-1), "\\") != 0)
    {
		fprintf(stderr, "[ERR]  Input path must end with a \\\n");
#else
    if(strcmp((char*)(argv[1]+strlen(argv[1])-1), "/") != 0)
    {
		fprintf(stderr, "[ERR]  Input path must end with a /\n");
#endif
		return 2;
    }

	if((outfile = fopen(argv[2], "wb+")) == NULL)
    {
		fprintf(stderr, "[ERR]  Cannot open %s\n", argv[2]);
		return 3;
	}
    
    fseek(outfile, 0x40, SEEK_SET);
    
    merge_hxf(argv[1], outfile, "");
    
    fflush(outfile);
    
    fprintf(stderr, "[INFO] Filling header...\n");
    
#undef WRITE
#define WRITE(x, len) if(fwrite(x, len, 1, outfile) != 1) \
                      { \
                            fprintf(stderr, "[ERR]  Cannot write to %s\n", argv[1]); \
                            fclose(outfile); \
                            return 4; \
                      }
    fflush(outfile);
    fseek(outfile, 0, SEEK_SET);
    WRITE("WADF0100200804111437", 20);
    WRITE(int2le(_filesize(outfile)), 4);
    WRITE(int2le(checksum(outfile)), 4);
    WRITE(int2le(0), 4);
    WRITE("Chinachip PMP firmware V1.0\0\0\0\0\0", 32);
    fclose(outfile);
    
    fprintf(stderr, "[INFO] Done!\n");
    
    return 0;
}
