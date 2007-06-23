 // bin2c.c
 //
 // convert a binary file into a C source vector
 //
 // put into the public domain by Sandro Sigala
 //
 // syntax:  bin2c [-c] [-z] <input_file> <output_file>
 //
 //          -c    add the "const" keyword to definition
 //          -z    terminate the array with a zero (useful for embedded C strings)
 //
 // examples:
 //     bin2c -c myimage.png myimage_png.cpp
 //     bin2c -z sometext.txt sometext_txt.cpp
 
 #include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 #ifndef PATH_MAX
 #define PATH_MAX 1024
 #endif
 
 int useconst = 0;
 int zeroterminated = 0;
 
 int myfgetc(FILE *f)
 {
 	int c = fgetc(f);
 	if (c == EOF && zeroterminated) {
 		zeroterminated = 0;
 		return 0;
 	}
 	return c;
 }
 
 
 void process(const char *ifname, const char *ofname)
 {
 	FILE *ifile, *ofile;
	/* modified */
	int counter=0;	
 	char buf2[PATH_MAX];
 	char* cp2;
 	char* cp3;
 	if ((cp3 = strrchr(ofname, '/')) != NULL)
 		++cp3;
 	else {
 		if ((cp3 = strrchr(ofname, '\\')) != NULL)
 			++cp3;
 		else
 			cp3 = (char*) ofname;
 	}
 	
 	strcpy(buf2, cp3);
 	cp2 = strrchr(buf2, '.');
 	*cp2 = '.';
 	cp2++;
 	*cp2 = 'h';
 	cp2++;
 	*cp2 ='\0';
 	
 	
 	ifile = fopen(ifname, "rb");
 	if (ifile == NULL) {
 		fprintf(stderr, "cannot open %s for reading\n", ifname);
 		exit(1);
 	}
 	ofile = fopen(ofname, "wb");
 	if (ofile == NULL) {
 		fprintf(stderr, "cannot open %s for writing\n", ofname);
 		exit(1);
 	}
 	char buf[PATH_MAX], *p;
 	const char *cp;
 	if ((cp = strrchr(ifname, '/')) != NULL)
 		++cp;
 	else {
 		if ((cp = strrchr(ifname, '\\')) != NULL)
 			++cp;
 		else
 			cp = ifname;
 	}
 	strcpy(buf, cp);
 	for (p = buf; *p != '\0'; ++p)
 		if (!isalnum(*p))
 			*p = '_';
	fprintf(ofile,"#include \"%s\" \n\n",buf2);
 	fprintf(ofile, "%sunsigned char %s[] = {\n", useconst ? "const " : "", buf);
 	int c, col = 1;
 	while ((c = myfgetc(ifile)) != EOF) {
         counter++;
 		if (col >= 78 - 6) {
 			fputc('\n', ofile);
 			col = 1;
 		}
 		fprintf(ofile, "0x%.2x, ", c);
 		col += 6;
 
 	}
 	fprintf(ofile, "\n};\n");
 	
 	/* modified */
 	fprintf(ofile,"int %s_length = %i; \n",buf,counter);
 	
 	
 	FILE *o2file;
 	o2file = fopen(buf2, "wb");
    	if (o2file == NULL) {
 		fprintf(stderr, "cannot open %s for writing\n", buf2);
 		exit(1);
 	}
 	
 	fprintf(o2file, "#ifndef __%s__ \n", buf);
 	fprintf(o2file, "#define __%s__ \n", buf);
  	
 	fprintf(o2file, "extern %sunsigned char %s[]; \n\n", useconst ? "const " : "", buf);
 	fprintf(o2file, "extern int %s_length; \n\n", buf);
 	
 	fprintf(o2file, "#endif \n");
 	
 	fclose(ifile);
 	fclose(ofile);
 	fclose(o2file);
 }
 
 void usage(void)
 {
 	fprintf(stderr, "usage: bin2c <input_files> \n");
 	exit(1);
 }
 
 int main(int argc, char **argv)
 {
    if (argc < 2) {
 		usage();
 	}
 	int i;
 	for(i = 1;i < argc ; i++)
 	{
       char buf[PATH_MAX];
 	   char* cp;
 	   strcpy(buf, argv[i]);
 	   cp = strrchr(buf, '.');
 	   cp++;
 	   strcpy(cp,"cpp");
 	   process(argv[i], buf);    
    }
     
     
    /* 
 	while (argc > 3) {
 		if (!strcmp(argv[1], "-c")) {
 			useconst = 1;
 			--argc;
 			++argv;
 		} else if (!strcmp(argv[1], "-z")) {
 			zeroterminated = 1;
 			--argc;
 			++argv;
 		} else {
 			usage();
 		}
 	}
 	if (argc != 3) {
 		usage();
 	}
 	process(argv[1], argv[2]);
 	*/
 	return 0;
 }
