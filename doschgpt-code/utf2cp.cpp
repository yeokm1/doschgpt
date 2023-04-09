#include "utf2cp.h"
#include "utfcp437.h"
#include "utfcp737.h"

CONVERSION_OUTPUT utf_to_cp(int codepage, int value0, int value1, int value2){
    CONVERSION_OUTPUT output;

    if(value0 <= 127){
        output.character = value0;
        output.charactersUsed = 1;
        return output;
    }
    output.character = UNKNOWN_CHAR_REPLACEMENT;

    switch(value0){
        case 0xC2:
            if(codepage == CODE_PAGE_737){
                output.character = utf_to_cp737_c2(value1);
            } else {
                output.character = utf_to_cp437_c2(value1);
            }
            output.charactersUsed = 2;
            break;
        case 0xC3:
            if(codepage == CODE_PAGE_737){
                output.character = utf_to_cp737_c3(value1);
            } else {
                output.character = utf_to_cp437_c3(value1);
            }
            output.charactersUsed = 2;
            break;
        case 0xC6:
            output.character = utf_to_cp437_c6(value1);
            output.charactersUsed = 2;
            break;
        case 0xCE:
            if(codepage == CODE_PAGE_737){
                output.character = utf_to_cp737_ce(value1);
            } else {
                output.character = utf_to_cp437_ce(value1);
            }
            output.charactersUsed = 2;
            break;
        case 0xCF:
            if(codepage == CODE_PAGE_737){
                output.character = utf_to_cp737_cf(value1);
            } else {
                output.character = utf_to_cp437_cf(value1);
            }
            output.charactersUsed = 2;
            break;
        case 0xE2:
            if(codepage == CODE_PAGE_737){
                output.character = utf_to_cp737_e2(value1, value2);
            } else {
                output.character = utf_to_cp437_e2(value1, value2);
            }
            output.charactersUsed = 3;
            break;
        default:
            output.character = UNKNOWN_CHAR_REPLACEMENT;
            output.charactersUsed = 1;
    }

    return output;

}
