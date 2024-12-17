/* Note these are not ucschar_t becuase all arabic
   codepoints are <16bit, so no need to waste table space */
typedef struct {
    unsigned short isolated;
    unsigned short final;
    unsigned short medial;
    unsigned short initial;
} arab_t;

extern const arab_t zwj; /* zero-width joiner */
extern const arab_t lamaleph[]; /* lam-aleph ligatures */
extern const arab_t jointable[]; /* lookup table for arabic joining */
