#include "utfcp437.h"

CONVERSION_OUTPUT utf_to_cp437(int value0, int value1, int value2){
    CONVERSION_OUTPUT output;

    if(value0 <= 127){
        output.character = value0;
        output.charactersUsed = 1;
        return output;
    }

    switch(value0){
        case 0xC2:
            output.character = utf_to_cp437_c2(value1);
            output.charactersUsed = 2;
            break;
        case 0xC3:
            output.character = utf_to_cp437_c3(value1);
            output.charactersUsed = 2;
            break;
        case 0xC6:
            output.character = utf_to_cp437_c6(value1);
            output.charactersUsed = 2;
            break;
        case 0xCE:
            output.character = utf_to_cp437_ce(value1);
            output.charactersUsed = 2;
            break;
        case 0xCF:
            output.character = utf_to_cp437_cf(value1);
            output.charactersUsed = 2;
            break;
        case 0xE2:
            output.character = utf_to_cp437_e2(value1, value2);
            output.charactersUsed = 3;
            break;
        default:
            output.character = UNKNOWN_CHAR_REPLACEMENT;
            output.charactersUsed = 1;
    }

    return output;

}


unsigned char utf_to_cp437_c3(int value1){
    switch(value1){
        case 0x87: return 128;
        case 0xBC: return 129;
        case 0xA9: return 130;
        case 0xA2: return 131;
        case 0xA4: return 132;
        case 0xA0: return 133;
        case 0xA5: return 134;
        case 0xA7: return 135;
        case 0xAA: return 136;
        case 0xAB: return 137;
        case 0xA8: return 138;
        case 0xAF: return 139;
        case 0xAE: return 140;
        case 0xAC: return 141;
        case 0x84: return 142;
        case 0x85: return 143;
        case 0x89: return 144;
        case 0xA6: return 145;
        case 0x86: return 146;
        case 0xB4: return 147;
        case 0xB6: return 148;
        case 0xB2: return 149;
        case 0xBB: return 150;
        case 0xB9: return 151;
        case 0xBF: return 152;
        case 0x96: return 153;
        case 0x9C: return 154;
        case 0xA1: return 160;
        case 0xAD: return 161;
        case 0xB3: return 162;
        case 0xBA: return 163;
        case 0xB1: return 164;
        case 0x91: return 165;
        case 0x9F: return 225;
        case 0xB7: return 246;

        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

unsigned char utf_to_cp437_c2(int value1){
    switch(value1){
        case 0xA2: return 155;
        case 0xA3: return 156;
        case 0xA5: return 157;
        case 0xAA: return 166;
        case 0xBA: return 167;
        case 0xBF: return 168;
        case 0xAC: return 170;
        case 0xBD: return 171;
        case 0xBC: return 172;
        case 0xA1: return 173;
        case 0xAB: return 174;
        case 0xBB: return 175;
        case 0xB5: return 230;
        case 0xB1: return 241;
        case 0xB0: return 248;
        case 0xB7: return 250;
        case 0xB2: return 253;
        case 0xA0: return 255;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

unsigned char utf_to_cp437_e2(int value1, int value2){

    if(value1 == 0x82){
        switch(value2){
            case 0xA7: return 158;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if(value1 == 0x96){
        switch(value2){
            case 0x91: return 176;
            case 0x92: return 177;
            case 0x93: return 178;
            case 0x88: return 219;
            case 0x84: return 220;
            case 0x8C: return 221;
            case 0x90: return 222;
            case 0x80: return 223;
            case 0xA0: return 254;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if(value1 == 0x94){
        switch(value2){
            case 0x82: return 179;
            case 0xA4: return 180;
            case 0x90: return 191;
            case 0x94: return 192;
            case 0xB4: return 193;
            case 0xAC: return 194;
            case 0x9C: return 195;
            case 0x80: return 196;
            case 0xBC: return 197;
            case 0x98: return 217;
            case 0x8C: return 218;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if(value1 == 0x95){
        switch(value2){
            case 0xA1: return 181;
            case 0xA2: return 182;
            case 0x96: return 183;
            case 0x95: return 184;
            case 0xA3: return 185;
            case 0x91: return 186;
            case 0x97: return 187;
            case 0x9D: return 188;
            case 0x9C: return 189;
            case 0x9B: return 190;
            case 0x9E: return 198;
            case 0x9F: return 199;
            case 0x9A: return 200;
            case 0x94: return 201;
            case 0xA9: return 202;
            case 0xA6: return 203;
            case 0xA0: return 204;
            case 0x90: return 205;
            case 0xAC: return 206;
            case 0xA7: return 207;
            case 0xA8: return 208;
            case 0xA4: return 209;
            case 0xA5: return 210;
            case 0x99: return 211;
            case 0x98: return 212;
            case 0x92: return 213;
            case 0x93: return 214;
            case 0xAB: return 215;
            case 0xAA: return 216;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if (value1 == 0x88){
        switch(value2){
            case 0x9E: return 236;
            case 0xA9: return 239;
            case 0x99: return 249;
            case 0x9A: return 251;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if (value1 == 0x89){
        switch(value2){
            case 0xA1: return 240;
            case 0xA5: return 242;
            case 0xA4: return 243;
            case 0x88: return 247;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if (value1 == 0x8C){
        switch(value2){
            case 0x90: return 169;
            case 0xA0: return 244;
            case 0xA1: return 245;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if (value1 == 0x81){
        switch(value2){
            case 0xBF: return 252;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    }

    return value1;
}

unsigned char utf_to_cp437_ce(int value1){
    switch(value1){
        case 0xB1: return 224;
        case 0x93: return 226;
        case 0xA3: return 228;
        case 0xA6: return 232;
        case 0x98: return 233;
        case 0xA9: return 234;
        case 0xB4: return 235;
        case 0xB5: return 238;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

unsigned char utf_to_cp437_cf(int value1){
    switch(value1){
        case 0x80: return 227;
        case 0xA3: return 228;
        case 0x83: return 229;
        case 0x84: return 231;
        case 0x86: return 237;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

unsigned char utf_to_cp437_c6(int value1){
    switch(value1){
        case 0x92: return 159;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

