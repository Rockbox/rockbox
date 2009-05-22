#include <stdio.h>
#include <stdlib.h>

#include "dmalloc.h"

#define MAX 500
#define MAX2 1000
#define MAXC 2

#define TESTA
#define TESTB
#define TESTC
#define TESTD

int main(int argc, char **argv)
{
  int i;
  int memory = 0;

#ifdef BMALLOC
#define HEAP 10000
  void *heap = (malloc)(HEAP);
  if(!heap)
    return -1;
  add_pool(heap, HEAP);
#endif

  {
#define MAXK 100
    void *wow[MAXK];
    wow[0]=malloc(700);
    realloc(wow[0], 680);
    return 0;
    
    for(i=0; i<MAXK; i++)
      if(!(wow[i]=malloc(412))) {
	printf("*** Couldn't allocated memory, exiting\n");
	return -2;
      }
    for(i=MAXK-1; i>=0; i-=2)
      free(wow[i]);
    return 0;
  }


#ifdef TESTD
  {
#define MAXS 10
#define MAXS1 0
    void *ptr[MAXS];

    for(i=MAXS1; i< MAXS; i++) {
      printf("%d malloc(%d)\n", i, i*55);
      ptr[i] = malloc (i*55);
    }
    for(i=MAXS1; i< MAXS; i++) {
      void *tmp;
      printf("%d realloc(%d)\n", i, i*155);
      tmp=realloc(ptr[i], i*155);
      if(tmp)
	ptr[i] = tmp;
    }
    for(i=MAXS1; i< MAXS; i++) {
      printf("%d free(%d)\n", i, i*155);
      free(ptr[i]);
    }
  }
#endif

#ifdef TESTC
  {
    void *ptr[MAXC];
    printf("This is test C:\n");
    
    for(i=0; i< MAXC; i++) {
      printf("%d malloc(100)\n", i+1);
      ptr[i] = malloc(100);
      printf(" ...returned %p\n", ptr[i]);
    }

    for(i=0; i< MAXC; i++) {
      printf("%d free()\n", i+1);
      free(ptr[i]);
    }

    printf("End of test C:\n");
  }
#endif

#ifdef TESTA
  {
    void *pointers[MAX];
    printf("This is test I:\n");

    for(i=0; i<MAX; i++) {
      printf("%d attempts malloc(%d)\n", i, i*6);
      pointers[i]=malloc(i*6);
      if(!pointers[i]) {
	printf("cant get more memory!");
	return(0);
      }
      memory += (i*6);
    }
    printf("\namount: %d\n", memory);
    memory = 0;
    for(i=0; i<MAX; i++) {
      printf("%d attempts realloc(%d)\n", i, i*7);
      pointers[i]=realloc(pointers[i], i*7);
      memory += i*7;
    }
    printf("\namount: %d\n", memory);
    for(i=0; i<MAX; i++) {
      printf("%d attempts free(%d)\n", i, i*7);
      free(pointers[i]);
    }
    printf("\nend of test 1\n");
  }
#endif
#ifdef TESTB
  {
    void *pointers2[MAX2];
    memory = 0;
    printf("\nTest II\n");
    for(i=0; i< MAX2; i++) {
/*      printf("%d attempts malloc(%d)\n", i, 7); */
      pointers2[i] = malloc(7);
      memory += 7;
    }
    printf("\namount: %d\n", memory);
    for(i=0; i< MAX2; i++) {
      free(pointers2[i]);
    }
    printf("\nend of test II\n");
    
  }
#endif
  return 0;
}
#include <stdio.h>
#include <stdlib.h>

#include "dmalloc.h"

#define MAX 500
#define MAX2 1000
#define MAXC 2

#define TESTA
#define TESTB
#define TESTC
#define TESTD

int main(int argc, char **argv)
{
  int i;
  int memory = 0;

#ifdef BMALLOC
#define HEAP 10000
  void *heap = (malloc)(HEAP);
  if(!heap)
    return -1;
  add_pool(heap, HEAP);
#endif

  {
#define MAXK 100
    void *wow[MAXK];
    wow[0]=malloc(700);
    realloc(wow[0], 680);
    return 0;
    
    for(i=0; i<MAXK; i++)
      if(!(wow[i]=malloc(412))) {
	printf("*** Couldn't allocated memory, exiting\n");
	return -2;
      }
    for(i=MAXK-1; i>=0; i-=2)
      free(wow[i]);
    return 0;
  }


#ifdef TESTD
  {
#define MAXS 10
#define MAXS1 0
    void *ptr[MAXS];

    for(i=MAXS1; i< MAXS; i++) {
      printf("%d malloc(%d)\n", i, i*55);
      ptr[i] = malloc (i*55);
    }
    for(i=MAXS1; i< MAXS; i++) {
      void *tmp;
      printf("%d realloc(%d)\n", i, i*155);
      tmp=realloc(ptr[i], i*155);
      if(tmp)
	ptr[i] = tmp;
    }
    for(i=MAXS1; i< MAXS; i++) {
      printf("%d free(%d)\n", i, i*155);
      free(ptr[i]);
    }
  }
#endif

#ifdef TESTC
  {
    void *ptr[MAXC];
    printf("This is test C:\n");
    
    for(i=0; i< MAXC; i++) {
      printf("%d malloc(100)\n", i+1);
      ptr[i] = malloc(100);
      printf(" ...returned %p\n", ptr[i]);
    }

    for(i=0; i< MAXC; i++) {
      printf("%d free()\n", i+1);
      free(ptr[i]);
    }

    printf("End of test C:\n");
  }
#endif

#ifdef TESTA
  {
    void *pointers[MAX];
    printf("This is test I:\n");

    for(i=0; i<MAX; i++) {
      printf("%d attempts malloc(%d)\n", i, i*6);
      pointers[i]=malloc(i*6);
      if(!pointers[i]) {
	printf("cant get more memory!");
	return(0);
      }
      memory += (i*6);
    }
    printf("\namount: %d\n", memory);
    memory = 0;
    for(i=0; i<MAX; i++) {
      printf("%d attempts realloc(%d)\n", i, i*7);
      pointers[i]=realloc(pointers[i], i*7);
      memory += i*7;
    }
    printf("\namount: %d\n", memory);
    for(i=0; i<MAX; i++) {
      printf("%d attempts free(%d)\n", i, i*7);
      free(pointers[i]);
    }
    printf("\nend of test 1\n");
  }
#endif
#ifdef TESTB
  {
    void *pointers2[MAX2];
    memory = 0;
    printf("\nTest II\n");
    for(i=0; i< MAX2; i++) {
/*      printf("%d attempts malloc(%d)\n", i, 7); */
      pointers2[i] = malloc(7);
      memory += 7;
    }
    printf("\namount: %d\n", memory);
    for(i=0; i< MAX2; i++) {
      free(pointers2[i]);
    }
    printf("\nend of test II\n");
    
  }
#endif
  return 0;
}
