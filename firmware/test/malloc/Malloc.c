#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Storleken på allokeringen bestäms genom att först slumpas en position i
"size_table" ut, sedan slumpas en storlek mellan den postionen och nästa värde
i tabellen.  Genom att ha tabellen koncentrerad med låga värden, så skapas
flest såna.  Rutinen håller på tills minnet en allokeringen nekas.  Den kommer
aldrig att ha mer än MAXIMAL_MEMORY_TO_ALLOCATE allokerat samtidigt.  Maximalt
har den MAX_ALLOCATIONS allokeringar samtidigt.

Statistiskt sätt så kommer efter ett tag MAX_ALLOCATIONS/2 allokeringar finnas
samtidigt, med varje allokering i median med värdet av halva "size_table".

När minnet är slut (malloc()=NULL), frågas användaren om han ska fortsätta.

Med jämna mellanrum skrivs statisktik ut på skärmen. (DISPLAY_WHEN)

För att stressa systemet med fler små allokeringar, så kan man öka
MAX_ALLOCATIONS.  AMOUNT_OF_MEMORY bör få den att slå i taket fortare om man
minskar det.

Ingen initiering görs av slumptalen, så allt är upprepbart (men plocka bort
kommentaren på srand() och det löser sig.

*/

/*#undef BMALLOC*/

#ifdef BMALLOC
#include "dmalloc.h"

#include "bmalloc.h"
#endif

#define MAX_ALLOCATIONS  100000
#define AMOUNT_OF_MEMORY 100000 /* bytes */
#define MAXIMAL_MEMORY_TO_ALLOCATE 49000 /* Sätt den här högre än
					    AMOUNT_OF_MEMORY, och malloc() bör
					    returnera NULL förr eller senare */

#define DISPLAY_WHEN (10000) /* When to display statistic */

#define min(a, b)  (((a) < (b)) ? (a) : (b))  
#define BOOL char
#define TRUE 1
#define FALSE 0

typedef struct {
  char *memory;
  long size;
  char filled_with;
  long table_position;
} MallocStruct;

/*
Skapar en lista med MAX_ALLOCATIONS storlek där det slumpvis allokeras 
eller reallokeras i.
*/

MallocStruct my_mallocs[MAX_ALLOCATIONS];

long size_table[]={5,8,10,11,12,14,16,18,20,26,33,50,70,90,120,150,200,400,800,1000,2000,4000,8000};
#define TABLESIZE ((sizeof(size_table)-1)/sizeof(long))
long size_allocs[TABLESIZE];

int main(void)
{
  long count=-1;
  long count_free=0, count_malloc=0, count_realloc=0;
  long total_memory=0;
  long out_of_memory=FALSE;

  dmalloc_initialize();

#ifdef BMALLOC
  void *thisisourheap;
  thisisourheap = (malloc)(AMOUNT_OF_MEMORY);
  if(!thisisourheap)
    return -1; /* can't get memory */
  bmalloc_add_pool(thisisourheap, AMOUNT_OF_MEMORY);
#endif

  srand( 0 ); /* Initialize to a fixed random */

  while (!out_of_memory) {
    long number=rand()%MAX_ALLOCATIONS;
    long size;
    long table_position=rand()%TABLESIZE;
    char fill_with=rand()&255;

    count++;

    size=rand()%(size_table[table_position+1]-
                 size_table[table_position])+
      size_table[table_position];
    
/*    fprintf(stderr, "number %d size %d\n", number, size); */

    if (my_mallocs[number].size) { /* Om allokering redan finns på den här
				      positionen, så reallokerar vi eller
				      friar. */
      long old_size=my_mallocs[number].size;
      if (my_mallocs[number].size && fill_with<40) {
        free(my_mallocs[number].memory);
        total_memory -= my_mallocs[number].size;
        count_free++;
	size_allocs[my_mallocs[number].table_position]--;
        size=0;
      } else {
	/*
	 * realloc() part
	 *
	 */
	char *temp;
#if 0
	if(my_mallocs[number].size > size) {
	  printf("*** %d is realloc()ed to %d\n",
		 my_mallocs[number].size, size);
	}
#endif
	if (total_memory-old_size+size>MAXIMAL_MEMORY_TO_ALLOCATE)
	  goto output; /* for-loop */
        temp = (char *)realloc(my_mallocs[number].memory, size);
	if (!temp)
	  out_of_memory=size;
	else {
	  my_mallocs[number].memory = temp;

	  my_mallocs[number].size=size;
	  size_allocs[my_mallocs[number].table_position]--;
	  size_allocs[table_position]++;
	  total_memory -= old_size;
	  total_memory += size;
	  old_size=min(old_size, size);
	  while (--old_size>0) {
	    if (my_mallocs[number].memory[old_size]!=my_mallocs[number].filled_with)
	      fprintf(stderr, "Wrong filling!\n");
	  }
	  count_realloc++;
	}
      }
    } else {
      if (total_memory+size>MAXIMAL_MEMORY_TO_ALLOCATE) {
	goto output; /* for-loop */
      }
      my_mallocs[number].memory=(char *)malloc(size); /* Allokera! */
      if (!my_mallocs[number].memory)
	out_of_memory=size;
      else {
	size_allocs[table_position]++;
	count_malloc++;
	total_memory += size;
      }
    }
    if(!out_of_memory) {
      my_mallocs[number].table_position=table_position;
      my_mallocs[number].size=size;
      my_mallocs[number].filled_with=fill_with;
      memset(my_mallocs[number].memory, fill_with, size);
    }
  output:    
    if (out_of_memory || !(count%DISPLAY_WHEN)) {
      printf("(%d)  malloc %d, realloc %d, free %d, total size %d\n",
             count, count_malloc, count_realloc, count_free, total_memory);
      {
	int count;
	printf("[size bytes]=[number of allocations]\n");
	for (count=0; count<TABLESIZE; count++) {
	  printf(" %ld=%ld\n", size_table[count], size_allocs[count]);
	}
	printf("\n\n");
      }
    }
    if (out_of_memory) {
      if(out_of_memory)
        printf("Couldn't get %d bytes\n", out_of_memory);

      dmalloc_status();
      bmalloc_status();

      fprintf(stderr, "Memory is out!  Continue (y/n)");
      switch (getchar()) {
      case 'y':
      case 'Y':
	out_of_memory=FALSE;
	break;
      }
      fprintf(stderr, "\n");
    }
  }
  printf("\n");
  return 0;
}

