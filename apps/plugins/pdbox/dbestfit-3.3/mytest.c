
#include <stdio.h>
#include <stdlib.h>

#include "bmalloc.h"

int main(int argc, char **argv)
{
  void *pointers[5];
  int i;
  void *area;

  for(i=0; i<5; i++)
    pointers[i] = malloc(8000);

  if(argc>1) {
    switch(argv[1][0]) {
    case '1':
      for(i=0; i<5; i++) {
	add_pool(pointers[i], 4000);
	add_pool((char *)pointers[i]+4000, 4000);
      }
      break;
    case '2':
      area = malloc(20000);
      add_pool(area, 3000);
      add_pool((char *)area+6000, 3000);
      add_pool((char *)area+3000, 3000);
      add_pool((char *)area+12000, 3000);
      add_pool((char *)area+9000, 3000);
      break;
    case '3':
      {
	void *ptr[10];
	area = malloc(20000);
	add_pool(area, 20000);

	printf(" ** TEST USAGE\n");
	for(i=0; i<9; i++)
	  ptr[i]=bmalloc(200);
	print_lists();
	for(i=0; i<9; i++)
	  bfree(ptr[i]);
	printf(" ** END OF TEST USAGE\n");
      }
      
      break;
    case '4':
      {
	void *ptr[10];
	area = malloc(20000);
	add_pool(area, 20000);

	ptr[0]=bmalloc(4080);
	print_lists();
	bfree(ptr[0]);
	printf(" ** END OF TEST USAGE\n");
      }
      
      break;
    }
  }
  else
    for(i=4; i>=0; i--)
      add_pool(pointers[i], 8000-i*100);

  print_lists();

  return 0;
}

#include <stdio.h>
#include <stdlib.h>

#include "bmalloc.h"

int main(int argc, char **argv)
{
  void *pointers[5];
  int i;
  void *area;

  for(i=0; i<5; i++)
    pointers[i] = malloc(8000);

  if(argc>1) {
    switch(argv[1][0]) {
    case '1':
      for(i=0; i<5; i++) {
	add_pool(pointers[i], 4000);
	add_pool((char *)pointers[i]+4000, 4000);
      }
      break;
    case '2':
      area = malloc(20000);
      add_pool(area, 3000);
      add_pool((char *)area+6000, 3000);
      add_pool((char *)area+3000, 3000);
      add_pool((char *)area+12000, 3000);
      add_pool((char *)area+9000, 3000);
      break;
    case '3':
      {
	void *ptr[10];
	area = malloc(20000);
	add_pool(area, 20000);

	printf(" ** TEST USAGE\n");
	for(i=0; i<9; i++)
	  ptr[i]=bmalloc(200);
	print_lists();
	for(i=0; i<9; i++)
	  bfree(ptr[i]);
	printf(" ** END OF TEST USAGE\n");
      }
      
      break;
    case '4':
      {
	void *ptr[10];
	area = malloc(20000);
	add_pool(area, 20000);

	ptr[0]=bmalloc(4080);
	print_lists();
	bfree(ptr[0]);
	printf(" ** END OF TEST USAGE\n");
      }
      
      break;
    }
  }
  else
    for(i=4; i>=0; i--)
      add_pool(pointers[i], 8000-i*100);

  print_lists();

  return 0;
}
