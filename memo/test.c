#include <stdio.h>
#include <stdlib.h>
#include "mot.h"

/* main */
int main(void) {
    FILE *fp;
    char *filename = "os.mot";
    int ch;
    uint8 *p;

    /* ファイルのオープン */
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "%sのオープンに失敗しました.\n", filename);
        exit(EXIT_FAILURE);
    }

    /* ファイルの終端まで文字を読み取り表示する */
    while (( ch = fgetc(fp)) != EOF ) {
      p=mot(ch);
    }

    /* ファイルのクローズ */
    fclose(fp);

    printf("%p\n",p);

    return EXIT_SUCCESS;
}
