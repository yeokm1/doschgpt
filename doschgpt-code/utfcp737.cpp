#include "utfcp737.h"
#define UNKNOWN_CHAR_REPLACEMENT 0xFE

unsigned char utf_to_cp737_ce(int value1){

    switch(value1){
        case 0x91: return 128;
        case 0x92: return 129;
        case 0x93: return 130;
        case 0x94: return 131;
        case 0x95: return 132;
        case 0x96: return 133;
        case 0x97: return 134;
        case 0x98: return 135;
        case 0x99: return 136;
        case 0x9A: return 137;
        case 0x9B: return 138;
        case 0x9C: return 139;
        case 0x9D: return 140;
        case 0x9E: return 141;
        case 0x9F: return 142;
        case 0xA0: return 143;
        case 0xA1: return 144;
        case 0xA3: return 145;
        case 0xA4: return 146;
        case 0xA5: return 147;
        case 0xA6: return 148;
        case 0xA7: return 149;
        case 0xA8: return 150;
        case 0xA9: return 151;
        case 0xB1: return 152;
        case 0xB2: return 153;
        case 0xB3: return 154;
        case 0xB4: return 155;
        case 0xB5: return 156;
        case 0xB6: return 157;
        case 0xB7: return 158;
        case 0xB8: return 159;
        case 0xB9: return 160;
        case 0xBA: return 161;
        case 0xBB: return 162;
        case 0xBC: return 163;
        case 0xBD: return 164;
        case 0xBE: return 165;
        case 0xBF: return 166;
        case 0xAC: return 225;
        case 0xAD: return 226;
        case 0xAE: return 227;
        case 0xAF: return 229;
        case 0x86: return 234;
        case 0x88: return 235;
        case 0x89: return 236;
        case 0x8A: return 237;
        case 0x8C: return 238;
        case 0x8E: return 239;
        case 0x8F: return 240;
        case 0xAA: return 244;
        case 0xAB: return 245;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
    
}

unsigned char utf_to_cp737_cf(int value1){

    switch(value1){
        case 0x80: return 167;
        case 0x81: return 168;
        case 0x82: return 170;
        case 0x83: return 169;
        case 0x84: return 171;
        case 0x85: return 172;
        case 0x86: return 173;
        case 0x87: return 174;
        case 0x88: return 175;
        case 0x89: return 224;
        case 0x8A: return 228;
        case 0x8C: return 230;
        case 0x8D: return 231;
        case 0x8B: return 232;
        case 0x8E: return 233;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }

}

unsigned char utf_to_cp737_c2(int value1){
    switch(value1){
        case 0xB1: return 241;
        case 0xB0: return 248;
        case 0xB7: return 250;
        case 0xB2: return 253;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

unsigned char utf_to_cp737_c3(int value1){
    switch(value1){
        case 0xB7: return 246;
        default: return UNKNOWN_CHAR_REPLACEMENT;
    }
}

unsigned char utf_to_cp737_e2(int value1, int value2){

    if(value1 == 0x96){
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
            case 0x99: return 249;
            case 0x9A: return 251;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if (value1 == 0x89){
        switch(value2){
            case 0xA5: return 242;
            case 0xA4: return 243;
            case 0x88: return 247;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    } else if (value1 == 0x81){
        switch(value2){
            case 0xBF: return 252;
            default: return UNKNOWN_CHAR_REPLACEMENT;
        }
    }

    return UNKNOWN_CHAR_REPLACEMENT;
}