#define UNKNOWN_CHAR_REPLACEMENT 0xFE

typedef struct
{
    //How many values was used during parsing
    int charactersUsed;

    //(Extended) ASCII code of the character. Using int to avoid padding compilation problem
    // If character cannot be matched, will return UNKNOWN_CHAR_REPLACEMENT
    unsigned int character;

} CONVERSION_OUTPUT;


//Provide up to 3 bytes of the string to attempt to merge to CP437 code page
CONVERSION_OUTPUT utf_to_cp437(int value0, int value1, int value2);

// Internally calls this to parse second characters starting with _cX
unsigned char utf_to_cp437_c2(int value1);
unsigned char utf_to_cp437_c3(int value1);
unsigned char utf_to_cp437_c6(int value1);
unsigned char utf_to_cp437_ce(int value1);
unsigned char utf_to_cp437_cf(int value1);

// 2nd character with e2 will require another value for parsing
unsigned char utf_to_cp437_e2(int value1, int value2);

