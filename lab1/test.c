#include <stdio.h>
#include <stdbool.h>
#include <string.h>

char a[10005], b[10005];
int c[10005], d[10005], e[10005], alen, blen, clen, i, j;

bool Equal(int d[], int e[], int i) {
    if (d[i + blen] != 0)
        return true;
    j = blen - 1;
loop6:
    if (j< 0 )
        goto loop6_end;
    //for (j = blen - 1; j >= 0; j--) {
    if (d[i + j] > e[j])
        return true;
    if (d[i + j] < e[j])
        return false;
    
    j--;
    goto loop6;
loop6_end:
    return true;
}

int main()
{
    char tmp;
input_a:
    tmp = getchar();
    if(tmp == '\n')
        goto input_b;
    a[alen] = tmp;
    alen++;
    goto input_a;
    
input_b:
    tmp = getchar();
    if(tmp == '\n')
        goto do_div;
    b[blen] = tmp;
    blen++;
    goto input_b;


    
do_div:
    //for (i = 0; i < alen; i++)
    i = 0;
loop1:
    if (i >= alen)
        goto loop1_end;
    d[alen - i - 1] = a[i] - '0';
    i++;
    goto loop1;
loop1_end:
    i = 0;
loop2:
    //for (i = 0; i < blen; i++)
    if (i >= blen)
        goto loop2_end;
    e[blen - i - 1] = b[i] - '0';
    i++;
    goto loop2;
loop2_end:
    clen = alen - blen + 1;
    //for (i = clen - 1; i >= 0; i--)
    i = clen - 1;
loop3:
    if (i < 0)
        goto loop3_end;



    //while (Equal(d, e, i)) {
loop4:
    if (!Equal(d,e,i))
        goto loop4_end;
    //for (j = 0; j < blen; j++) {
    j = 0;
loop5:
    if(j >= blen)
        goto loop5_end;
    d[i + j] -= e[j];
    if (d[i + j] < 0) {
        d[i + j] += 10;
        d[i + j + 1]--;
    }
    j++;
    goto loop5;
loop5_end:
    c[i]++;
    
    goto loop4;
loop4_end:
    i--;
    goto loop3;
    // for (i = clen-1; i>=0; i--)
    //     printf("%d", c[i]);
loop3_end:
    i = clen-1;
print:
    if (i<0)
        goto end;
    char ch = c[i]+'0';
    putchar(ch);
    i--;
    goto print;
end:
    return 0;
}

