%{
#include "memory.h"
#include "memory-page.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
void allocate (int);
void release (int);
void spy (int);
void dump (void);
void prompt (void);
%}

%token NUMBER
%token ALLOCATE
%token RELEASE
%token DUMP
%token SPY
%token CHECK
%token INIT
%token QUIT

%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%%
commands
  : command ';'
    { }
  | commands command ';' 
    { }
  | error ';'
    { yyerrok; }
  ;

command
  : allocate
  | release
  | spy
  | check
  | INIT
    { memory_setup (); }
  | DUMP
    { dump (); }
  | QUIT
    { return 0; }
  ;

allocate
  : ALLOCATE expression
    { allocate (yylval); }
  ;

release
  : RELEASE expression
    { release (yylval); }
  ;

spy
  : SPY expression
    { spy (yylval); }
  ;

check
  : CHECK expression
    { __memory_check (yylval); }
  ;

expression
  : expression '+' expression
    { $$ = $1 + $3; }
  | expression '-' expression
    { $$ = $1 - $3; }
  | expression '*' expression
    { $$ = $1 * $3; }
  | expression '/' expression
    {
      if($3 == 0)
        yyerror("divide by zero");
      else
        $$ = $1 / $3;
    }
  | '-' expression %prec UMINUS
    {
      $$ = -$2;
    }
  | '(' expression ')'
    {
      $$ = $2;
    }
  | NUMBER
    {
      $$ = $1;
    }
  ;
      
%%

#include <readline/readline.h>
#include <readline/history.h>

int yyerror(char *s)
  {
    fprintf(stderr,"\nBad command");
    return 1;
  }

void prompt (void)
  {
    printf("\n>"); fflush (stdout);
  }

void allocate (int order)
  {
    void *address;
    printf("\nallocating a page of %d bytes...",512<<order);
    if ((unsigned)order > 21)
      printf (" bad order !");
    else if ((address = memory_allocate_page (order)))
      printf (" page #%d allocated !",((char *)address - (char *)__memory_free_page) >> 9);
    else
      printf (" cannot allocate a page !");
  }

void release (int page)
  {
    void *address = (void *)((char *)__memory_free_page + (page << 9));
    printf("\nreleasing page #%d...",page);
    if ((unsigned)page >= (2*1024*1024/512))
      printf (" bad page number !");
    else if (memory_release_page (address))
      printf (" page #%d released !",page);
    else
      printf (" cannot release this page !");
  }

void spy (int page)
  {
    void *address = (void *)((char *)__memory_free_page + (page << 9));
    printf("\nspying page #%d...",page);
    if ((unsigned)page >= (2*1024*1024/512))
      printf (" bad page number !");
    else
      __memory_spy_page (address);
  }

void dump (void)
  {
    int order;
    printf("\ndumping free pages list...");
    for (order = 0; order < 13; ++order)
      __memory_dump (order);
  }

int main ()
  {
    yyparse();
    return 0;
  }

int read_input (char *buffer,int max)
  {
    char *line = 0;
    while (1)
      {
        line = readline ("\n>");     
        if (!line)
          break;
        if (*line)
          add_history(line);
	strncpy (buffer,line,max);
	strcat (buffer,";");
        free (line);
	return strlen (buffer);
      }
    buffer[0] = ';';
    return 1;
  }

