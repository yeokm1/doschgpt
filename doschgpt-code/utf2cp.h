#define UNKNOWN_CHAR_REPLACEMENT 0xFE

#define CODE_PAGE_737 737
#define CODE_PAGE_437 437

typedef struct
{
    //How many values was used during parsing
    int charactersUsed;

    //Page code of the character. Using int to avoid padding compilation problem
    // If character cannot be matched, will return UNKNOWN_CHAR_REPLACEMENT
    unsigned int character;

} CONVERSION_OUTPUT;


//Provide up to 3 bytes of the string to attempt to parse to code page
CONVERSION_OUTPUT utf_to_cp(int codepage, int value0, int value1, int value2);
