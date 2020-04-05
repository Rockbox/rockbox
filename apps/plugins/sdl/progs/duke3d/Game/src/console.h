#ifndef _CONSOLE_H_
#define _CONSOLE_H_


// Public member functions
void     CONSOLE_Init(void);
void     CONSOLE_Reset(void);
void     CONSOLE_Term(void);
void     CONSOLE_ParseStartupScript(void);
void     CONSOLE_HandleInput(void);
void     CONSOLE_Render(void);
void     CONSOLE_ParseCommand(char * command);
void     CONSOLE_Printf(const char  *newmsg, ...);
int      CONSOLE_GetArgc(void);
char *    CONSOLE_GetArgv(unsigned int var);
int      CONSOLE_IsActive(void);
void     CONSOLE_SetActive(int i);

#endif
