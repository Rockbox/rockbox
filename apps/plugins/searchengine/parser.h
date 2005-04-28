extern struct token *tokenbuffer,*currentToken;

extern int syntaxerror;
extern char errormsg[250];

unsigned char *parse(struct token *tokenbuf);
void parser_acceptIt(void);
int parser_accept(unsigned char kind);
unsigned char *parseCompareNum(void);
unsigned char *parseCompareString(void);
unsigned char *parseExpr(void);
unsigned char *parseMExpr(void);
