
#include "scalarmult.ih"
#include "scalarmult.h"
//clear point
void set0(int32_t *dst)
{
    unsigned idx;
    
    for (idx = 0; idx < 10; ++idx)
    dst[idx] = 0;
}

//Square x, n times
void sqrn (int32_t* y, const int32_t* x, int n)
{
    int32_t tmp[10];

    if (n&1)
    {
     sqr(y,x);
     n--;
    }
    else
    {
     sqr(tmp,x);
     sqr(y,tmp);
     n-=2;
    }

    for (; n; n-=2)
    {
     sqr(tmp,y);
     sqr(y,tmp);
    }
}

//Hash2Point
//r is 32 bytes hash
//point in Montgomery format
void r2p (unsigned char* p, const unsigned char* r)
{
    int32_t L0[10], L1[10], L2[10], L3[10], Lx[10], Ly[10];
    short res;

    //clear two MSB
    for(res=0;res<8;res++) ((unsigned int*)L1)[res]=((unsigned int*)r)[res];
    ((unsigned char*)L1)[31]&=0x3F;
    from_bytes(L0, (unsigned char*)L1); //L0=r

    //compute point candidate x=-A/(2r^2+1)
    set0(L1); L1[0]=1;  //L1=1
    sqr (Lx, L0);   //Lx=r^2
    add (Lx, Lx, Lx); //Lx=2r^2
    add (Lx, Lx, L1); //Lx=2r^2+1
    invert(Lx, Lx); //Lx=1/(2r^2+1)
    set0(L1); L1[0]=486662;  //L1=A
    mul(Lx, Lx, L1); //Lx=A/(2r^2+1)
    set0(L1); //L1=0
    sub(Lx, L1, Lx); //Lx=-A/(2r^2+1)
    //point candidate x is in Lx

    //y^2 = x^3 + Ax^2 + x
    sqr (L2, Lx);
    mul (L3, L2, Lx);
    add (L1, L3, Lx);
    set0(Ly); Ly[0]=486662;  //Ly=A
    mul(L2, L2, Ly);
    add (Ly, L1, L2);

    //try inverse square root from y^2
    sqr (L0, Ly);
    mul (L1, L0, Ly);
    sqr (L0, L1);
    mul (L1, L0, Ly);
    sqrn(L0, L1, 3);
    mul (L2, L0, L1);
    sqrn(L0, L2, 6);
    mul (L1, L2, L0);
    sqr (L2, L1);
    mul (L0, L2, Ly);
    sqrn(L2, L0, 12);
    mul (L0, L2, L1);
    sqrn(L2, L0, 25);
    mul (L3, L2, L0);
    sqrn(L2, L3, 25);
    mul (L1, L2, L0);
    sqrn(L2, L1, 50);
    mul (L0, L2, L3);
    sqrn(L2, L0, 125);
    mul (L3, L2, L0);
    sqrn(L2, L3, 2);
    mul (L0, L2, Ly);
    sqr (L2, L0);
    mul (L3, L2, Ly);

    //can be 1, -1 or sqrt(-1) or -sqrt(-1), only 1 or -1 are valid points
    to_bytes((uint8_t*)L2, L3);
    res = 1 &  (    ( (uint16_t*)L2 )[1] ^ ( (uint16_t*)L2)[2]     );

    //select x or -x-A
    set0(L1); //L1=0
    sub(L0, L1, Lx);  //L0=-x
    cswap(L0, Lx, res); //Lx is x or -x
    set0(L0);
    set0(L1); L1[0]=486662;  //Ly=A
    cswap(L1, L0, res);  //L0 is 0 or A
    sub (Lx, Lx, L0);  //Lx is x or -x-A
    to_bytes(p, Lx); //output point
}

void scalarmultbase(unsigned char *p, const unsigned char *n)
{
 int32_t BP[10];

 set0(BP); BP[0]=9;  //BP=9
 scalarmult(p, n, (unsigned char*)BP);
}
