#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#define FALSE       0
#define TRUE        (!FALSE)
#define EMPTY       0      /* 駒の無い所 */
#define WHITE      -1      /* 白の駒 */
#define BLACK       1      /* 黒の駒 */
#define SWAPT(tp,a,b)   { tp w; w=a; a=b; b=w; }

typedef struct {
    char xd;
    char yd;
} course;

course direct[] = {
    { 0,  0},
    { 0, -1},
    { 1,  0},
    { 0,  1},
    {-1,  0},
    { 1, -1},
    { 1,  1},
    {-1,  1},
    {-1, -1},
};                /* 盤面を読みに行く方向テーブル */

typedef struct {
    char dat[10][10];
} history;
static history result[2048]={[0 ... 2047].dat = 
                             {{0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,WHITE,BLACK,0,0,0,0},
                              {0,0,0,0,BLACK,WHITE,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0},
                              {0,0,0,0,0,0,0,0,0,0}}
};  /* パーフェクト局面の履歴用 */

typedef struct {
    int xp;
    int yp;
    int cnt;
    history kmdat;
} scut;

int count;          /* パーフェクト局面のカウンター */
int completeflg;    /* パーフェクトかどうか */
int shortflg;       /* １通り発見次第ループを抜けるかどうか */
char compu,user;    /* 後手（白の駒）,先手（黒の駒） */

void insertion (scut km[], int n);
int keffect (char k1, char k2, history *p1);
void kdisp (history *tmp);
void brain (char compxuser, history *p1, int depth);
int lookahead (history *p1, char cnt, char escflg, int precan, char limit);
int kjudge (char compxuser, int xx, int yy, history *p1);
void getclk(char ti[]);

int main(int argc, char *argv[])
{
    history koma = {.dat =
                    {{0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,WHITE,BLACK,0,0,0,0},
                     {0,0,0,0,BLACK,WHITE,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0}}
    };
    char ti[9];
    history tmp = {.dat = {{0}}};
    int depth = 7;     /*  読む深さ (8 = 10手読み) 固定で確保しているパーフェクト局面保存用配列の制限で
                        depth = 9 までが限界 */
    char para[] = "X";

    if ((argc > 2) || (argc == 2 && strstr(argv[1],para) == NULL)) {
	fprintf(stderr, "Usage: %s [X]\n",argv[0]);
	exit(1);
    }
    if ((argc == 2) && ((strstr(argv[1],para)) != NULL)){
        shortflg = FALSE;   /* 枝刈りは行わず、パーフェクト局面を発見後
                               その手数での全パターンを保存 */
    } else {
        shortflg = TRUE;    /* 枝刈りを行って、パーフェクト局面を１通りでも
                               発見次第読むのを止める */
    }
    user = BLACK;
    compu = WHITE;
    printf("\x1b[2J");
    kdisp(&koma);
    getclk(ti);
    fprintf(stderr,"%s\n", ti);

    kjudge(user, 4, 3, &koma);       /* 回転解を省くため１手目は固定 */
    tmp = koma;              /* １手後を初期データとして保存 */
    result[count].dat[4][3] = '1';  /* １手目の履歴保存 */

    count = 0;
    completeflg = FALSE;
    koma = tmp;
    brain(compu, &koma, depth);
    getclk(ti);
    fprintf(stderr,"%s\n", ti);
    if (shortflg == FALSE){
        printf("%d 通り\n", count);
    }
    return 0;
}

void brain (char compxuser, history *p1, int depth)
{
    int i,j;
    int effect = 0;        /* 評価値を格納 */
    int cnt = 1;           /* 再帰用カウンター */
    int dummy = INT_MIN;   /* 評価値比較用ダミー */
    history *tmp;
    tmp = malloc(sizeof(history));
    if (tmp == NULL){
        fprintf(stderr,"Not Enough Memory 1\n");
        exit (1);
    }
    memcpy(tmp, p1, sizeof(history));
    
    for (j = 1;j <= 8;j++) {
        for (i = 1;i <= 8;i++) {
            *p1 = *tmp;
            if (kjudge(compxuser, i, j, p1) > 0) {
                result[count].dat[i][j] = '2';    /* ２手目の履歴保存 */
                effect = lookahead(p1, cnt, dummy, TRUE, depth);  /* 先読み */
                if ((effect != 0) && (shortflg == TRUE)){
                    break; /* パ-フェクト局面発見次第抜ける */
                }
            }
        }
        if ((effect != 0) && (shortflg == TRUE))
            break; /* パ-フェクト局面発見次第抜ける */
    }
    *p1 = *tmp;
    free (tmp);
}

int keffect (char k1, char k2, history *p1)
{
    int i, j;
    int perfct1 = 0;
    int perfct2 = 0;
    for (j = 1;j <= 8;j++) {
        for (i = 1;i <= 8;i++) {
            if (p1->dat[i][j] == k1) {
                perfct1 += 1;
                if (perfct2 != 0) {
                    return 0; /* パーフェクトじゃない */
                }
            }
            else if (p1->dat[i][j] == k2) {
                perfct2 += 1;
                if (perfct1 != 0) {
                    return 0; /* パーフェクトじゃない */
                }
            }
        }
    }
    if (perfct1 == 0)
        return 1;   /* compuのパーフェクト勝ち */
    else if (perfct2 == 0)
        return 1;   /* userのパーフェクト勝ち。
                       オセロのプログラムなら最小値を入れるべき部分 */
    else
        return 0;   /* 絶対来ない 
                       オセロのプログラムなら、その局面の評価値が
                       入るべき部分 */
}

void kdisp (history *tmp)
{
    int i,j;
    printf("\n");
    for (j = 1; j<=8;j++) {
        for (i = 1;i<=8;i++) {
            switch(tmp->dat[i][j]) {
            case EMPTY:
                printf("| ");
                break;
            case WHITE:
                printf("|W");
                break;
            case BLACK:
                printf("|B");
                break;
            default:
                printf("|%c",tmp->dat[i][j]); 
                break;
            }
        }
        printf("|\n");
    }
}

void insertion (scut km[], int n)
{
    int i, j;
    for (i = 2; i <= n; i++){
	j = i;
	while ((j > 1) && (km[j - 1].cnt > km[j].cnt)){
            SWAPT (scut, km[j - 1], km[j]);
	    j --;
	}
    }
}

int lookahead (history *p1, char cnt, char escflg, int precan, char limit)
{
    int  i, k, l, last;
    int turned;
    int procnt = 0;
    int canputflg;  /* ある局面で駒を置くことが出来たかどうか */
    int effect = 0; /* 評価値を格納 */
    int dum = 0;    /* 同じ深さでの評価値の比較用 */
    scut *tmp2;
    history *tmp;

    tmp = malloc(sizeof(history));
    if (tmp == NULL){
        fprintf(stderr,"Not Enough Memory 2\n");
        exit (1);
    }
    memcpy(tmp, p1, sizeof(history));

    tmp2 = malloc(sizeof(scut));
    if (tmp2 == NULL){
        fprintf(stderr,"Not Enough Memory 3\n");
        exit (1);
    }

    canputflg = FALSE;
    if ((cnt % 2) == 1){
         dum = INT_MAX;  /* userの手番 */
    } else {
         dum = INT_MIN;  /* compuの手番 */
    }
    for (l = 1;l <= 8;l++) {
        for (k = 1;k <= 8;k++) {
            if (p1->dat[k][l] != EMPTY) continue;
            switch (cnt % 2) {
                case 1:
                    turned = kjudge(user, k, l, p1);
                    break;
                case 0:
                    turned = kjudge(compu, k, l, p1);
                    break;
            }
            if (turned > 0){
                canputflg = TRUE;
                procnt += 1;
                tmp2 = realloc(tmp2, sizeof(scut) * (procnt + 1)) ;
                if (tmp2 == NULL){
                    fprintf(stderr,"Not Enough Memory 4\n");
                    exit (1);
                }
                tmp2[procnt].xp = k;
                tmp2[procnt].yp = l;
                tmp2[procnt].cnt = keffect(compu, user, p1);
                tmp2[procnt].kmdat = *p1;
                insertion(tmp2, procnt);
                *p1 = *tmp;
            }
        }
    }
    if ((cnt >= limit) || (precan == FALSE && canputflg == FALSE)){
        if (canputflg == FALSE){
            effect = keffect(compu, user, p1);
        } else {
            effect = keffect(compu, user, &tmp2[procnt].kmdat);
        }
        if (effect != 0){
            completeflg = TRUE;   /* パーフェクト */
            result[count + 1] = result[count];
            if (canputflg != FALSE){ /* 最後の一手は履歴には反映されていないのでセット */
              result[count].dat[tmp2[procnt].xp][tmp2[procnt].yp] = 
                ((cnt + 2) < 10) ? (cnt + 2 + '0') : (cnt - 8 + 'a');
            }
            kdisp (&result[count]);
            printf("%d  Answer = %d手目\n", count + 1, cnt + 2);
            count += 1;
        } else {
            completeflg = FALSE;  /* パーフェクトじゃない */
        }
        free(tmp);
        free(tmp2);
        return(effect);
    } else {
        if ((shortflg == TRUE) && (count != 0)){
            free (tmp);
            free (tmp2);
            return 1; /* パ-フェクト局面発見次第抜ける */
        }
        if (canputflg == FALSE){
            last = lookahead(p1, cnt + 1, dum, canputflg, limit);
        } else {
            for (i = procnt;i >= 1;i--){
                result[count].dat[tmp2[i].xp][tmp2[i].yp] = 
                ((cnt + 2) < 10) ? (cnt + 2 + '0') : (cnt - 8 + 'a');
                effect = lookahead(&tmp2[i].kmdat, cnt + 1, dum, canputflg, limit);
                result[count].dat[tmp2[i].xp][tmp2[i].yp] = EMPTY;
                if ((cnt % 2) == 1){
                    if ((effect < escflg) && (shortflg == TRUE)){
                        last = effect;
                        break;
                    } else if (effect < dum) {
                        last = effect;
                        dum = effect;
                    }
                } else {
                    if ((effect > escflg) && (shortflg == TRUE)){
                        last = effect;
                        break;
                    } else if (effect > dum) {
                        last = effect;
                        dum = effect;
                    }
                }
            }
        }
        free (tmp);
        free (tmp2);
        return last;
    }
}

int kjudge (char compxuser, int xx, int yy, history *p1)
{
    int i;
    int gentenx, genteny; /* 駒を打とうとした位置を保存 */
    int kazu = 0;         /* 裏返った駒の数 */
    gentenx = xx;
    genteny = yy;

    if ((xx < 1) || (8 < xx) || (yy < 1) || (8 < yy) || (p1->dat[xx][yy] != EMPTY))
        return 0;   /* 既に駒があればキャンセル */

    for (i = 1;i<=8;i++) {
        do{
            xx += direct[i].xd;
            yy += direct[i].yd;
            if (p1->dat[xx][yy] == EMPTY) {
                xx = gentenx; yy = genteny;
                break;
            }
        }while (p1->dat[xx][yy] != compxuser); /* 自分の駒か、盤の枠に当たるまで
                                        進んで、裏返しながら出発点に戻る */
        while (xx != gentenx || yy != genteny) {
            xx -= direct[i].xd;
            yy -= direct[i].yd;
            if ((xx == gentenx) && (yy == genteny))
                break;
            else{
                p1->dat[xx][yy] = compxuser;
                kazu += 1;
            }
        }
    }
    if (kazu > 0){
        p1->dat[gentenx][genteny] = compxuser;  /* 裏返った駒があれば、
　　　　　　　　　　　　　　　　　　　　　 　そこに駒を置く */
    }
    return kazu;     /* 裏返った駒の数を返す */
}

void getclk(char ti[])
{
        struct tm now_tm;
        time_t now_time;
        time(&now_time);
        now_tm = *localtime(&now_time);
        sprintf(ti, "%02d:%02d:%02d",
                now_tm.tm_hour,
                now_tm.tm_min,
                now_tm.tm_sec);
}
