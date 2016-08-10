#ifndef __PROGTEST__
#include <iostream>
// other header files
using namespace std;

bool UTF8toUTF16(const char * src, const char * dst);
bool UTF16toUTF8(const char * src, const char * dst);


#include <fstream>

#endif /* __PROGTEST__ */


typedef unsigned char byte;

typedef unsigned long cp;

int getNumOfBytesUTF8(byte b) {
    if (b < 0x80)
        return 1;
    if (b >= 0xC0 && b <= 0xDF)
        return 2;
    if (b >= 0xE0 && b <= 0xEF)
        return 3;
    if (b >= 0xF0 && b <= 0xF7)
        return 4;

    return 0;
}

cp getUTF8codepoint(const byte buffer[6], int size) {
    cp codePoint = 0;
    if (size == 1) {
        codePoint = buffer[0];
    }
    if (size == 2) {
        codePoint += ((buffer[0] - 0xC0) << 6);
        codePoint += (buffer[1] - 0x80);
    }
    if (size == 3) {
        codePoint += ((buffer[0] - 0xE0) << 12);
        codePoint += ((buffer[1] - 0x80) << 6);
        codePoint += ((buffer[2] - 0x80));
    }
    if (size == 4) {
        codePoint += ((buffer[0] - 0xF0) << 18);
        codePoint += ((buffer[1] - 0x80) << 12);
        codePoint += ((buffer[2] - 0x80) << 6);
        codePoint += ((buffer[3] - 0x80));
    }

    return codePoint;
}

int isCPOK(cp CP) {
    if (CP >= 0xd800 && CP <= 0xdfff)
        return 0;

    if (CP >= ((1 << 20) + (1 << 16)))
        return 0;

//    if (CP == 0xFFFF || CP == 0xFFFE)
//        return 0;
    return 1;
}

int fileOK(FILE * f) {
    return (!feof(f) && !ferror(f));
}

#define PADLA fclose(dstFile);fclose(srcFile); return false;

bool UTF8toUTF16(const char * src, const char * dst) {
    FILE * srcFile = fopen(src, "r");
    if (srcFile == NULL)
        return false;

    FILE * dstFile = fopen(dst, "w");
    if (dstFile == NULL) {
        fclose(srcFile);
        return false;
    }

    byte buffer[4];

    while (1) {
        int c = fgetc(srcFile);
        if (c == EOF) {
            break;
        }
        buffer[0] = c;

        int size = getNumOfBytesUTF8(buffer[0]);

        if (size == 0) {
            PADLA
        }

        for (int i = 1; i < size; i++) {
            c = fgetc(srcFile);
            if (c == EOF || c < 0x80 || c > 0xBF) {
                PADLA
            }
            buffer[i] = c;

        }
        if (!fileOK(srcFile)) {
            PADLA
        }

        cp codePoint = getUTF8codepoint(buffer, size);
        if (!isCPOK(codePoint)) {
            PADLA
        }
        //        printf("c: %d [%x]\n", (unsigned int) codePoint, (unsigned int) codePoint);
        //printf("size: %d \n", size);

        if (codePoint < 0x010000) {
            unsigned short tmp;
            tmp = codePoint;
            if (fwrite(&tmp, sizeof (tmp), 1, dstFile) != 1) {
                PADLA
            }
            //            dstFile.put((char) (tmp >> 8));
            //            dstFile.put(tmp);
        } else {
            codePoint = codePoint - 0x010000;
            unsigned short w1 = 0xD800;
            w1 = w1 + (codePoint >> 10);

            unsigned short w2 = codePoint & 0x03FF;
            w2 = w2 + 0xDC00;

            if ((fwrite(&w1, sizeof (w1), 1, dstFile) != 1) ||
                    (fwrite(&w2, sizeof (w2), 1, dstFile) != 1)) {
                PADLA
            }
        }
    }
    fclose(dstFile);
    fclose(srcFile);

    return true;
}

bool UTF16toUTF8(const char * src, const char * dst) {
    size_t result = 0;
    FILE * srcFile = fopen(src, "r");
    if (srcFile == NULL)
        return false;

    FILE * dstFile = fopen(dst, "w");
    if (dstFile == NULL) {
        fclose(srcFile);
        return false;
    }

    cp codePoint;
    while (1) {
        unsigned short w1, w2;
        result = fread(&w1, 1, 2, srcFile);
        if (result == 0) {
            break;
        } else if (result != 2) {
            PADLA
        }

        if (w1 < 0xD800 || w1 > 0xDFFF) {
            codePoint = w1;

        } else if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (fread(&w2, sizeof (w2), 1, srcFile) != 1) {
                PADLA
            }
            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                PADLA
            }
            codePoint = (w1 & 0x3ff);
            codePoint = codePoint << 10;
            codePoint += (w2 & 0x3ff);
            codePoint += 0x10000;
        } else {
            PADLA
        }

        if (!isCPOK(codePoint)) {
            PADLA
        }
        byte b1, b2, b3, b4;
        if (codePoint < 0x80) {

            b1 = codePoint;
            if (fwrite(&b1, sizeof (b1), 1, dstFile) != 1) {
                PADLA
            }
        } else if (codePoint < 0x800) {

            b1 = ((codePoint >> 6) & 0x3F) | 0xC0;
            b2 = (codePoint & 0x3F) | 0x80;
            if (fwrite(&b1, sizeof (b1), 1, dstFile) != 1) {
                PADLA;
            }
            if (fwrite(&b2, sizeof (b2), 1, dstFile) != 1) {
                PADLA;
            }
        } else if (codePoint < 0x10000) {

            b1 = ((codePoint >> 12) & 0xF) | 0xE0;
            b2 = ((codePoint >> 6) & 0x3F) | 0x80;
            b3 = (codePoint & 0x3F) | 0x80;

            if (fwrite(&b1, sizeof (b1), 1, dstFile) != 1) {
                PADLA;
            }

            if (fwrite(&b2, sizeof (b2), 1, dstFile) != 1) {
                PADLA;
            }

            if (fwrite(&b3, sizeof (b3), 1, dstFile) != 1) {
                PADLA;
            }
        } else if (codePoint < 0x200000) {
            b1 = ((codePoint >> 18) & 0x7) | 0xF0;
            b2 = ((codePoint >> 12) & 0x3F) | 0x80;
            b3 = ((codePoint >> 6) & 0x3F) | 0x80;
            b4 = (codePoint & 0x3F) | 0x80;
            if (fwrite(&b1, sizeof (b1), 1, dstFile) != 1) {
                PADLA;
            }
            if (fwrite(&b2, sizeof (b2), 1, dstFile) != 1) {
                PADLA;
            }
            if (fwrite(&b3, sizeof (b3), 1, dstFile) != 1) {
                PADLA;
            }
            if (fwrite(&b4, sizeof (b4), 1, dstFile) != 1) {
                PADLA;
            }
        }


    }
    fclose(dstFile);
    fclose(srcFile);

    return true;
}

#ifndef __PROGTEST__

int main(void) {
    FILE * out = fopen("in", "w");

    int c = getchar();
    while (!feof(stdin)) {

        fputc(c, out);
        c = getchar();
    }
    fclose(out);
    if (UTF8toUTF16("in", "out")) {
    //if (UTF8toUTF16("test.txt", "16.txt")) {
        printf("OK\n");
    } else {
        printf("CHYBA!!\n");
//        FILE * in = fopen("16.txt", "r");
//        while (!feof(in)) {
//            int c = fgetc(in);
//            putchar(c);
//        }
    }
    return ( 0);
}
#endif /* __PROGTEST__ */

