%{
#include "test.tab.h"
#define YY_INPUT(buf,result,max_size) \
  result = read_input (buf,max_size);
%}

%s GETNUMBER

%%

<GETNUMBER>[0-9]+       { yylval = atoi(yytext); return NUMBER;}

<INITIAL>"a"|"allocate" { BEGIN GETNUMBER; return ALLOCATE;    }
<INITIAL>"r"|"release"  { BEGIN GETNUMBER; return RELEASE;     }
<INITIAL>"s"|"spy"      { BEGIN GETNUMBER; return SPY;         }
<INITIAL>"c"|"check"    { BEGIN GETNUMBER; return CHECK;       }
<INITIAL>"i"|"init"     { return INIT;                         }
<INITIAL>"d"|"dump"     { return DUMP;                         }
<INITIAL>"q"|"quit"     { return QUIT;                         }
[ \t]                   ;
\n|.			{ BEGIN 0; return yytext[0];           }
%%

