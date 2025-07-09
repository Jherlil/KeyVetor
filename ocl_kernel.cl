__constant uint K[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

uint rotr(uint x,uint n){return (x>>n)|(x<<(32-n));}
uint Ch(uint x,uint y,uint z){return (x&y)^(~x&z);} 
uint Maj(uint x,uint y,uint z){return (x&y)^(x&z)^(y&z);} 
uint Sig0(uint x){return rotr(x,2)^rotr(x,13)^rotr(x,22);} 
uint Sig1(uint x){return rotr(x,6)^rotr(x,11)^rotr(x,25);} 
uint sig0(uint x){return rotr(x,7)^rotr(x,18)^(x>>3);} 
uint sig1(uint x){return rotr(x,17)^rotr(x,19)^(x>>10);} 

__kernel void sha256_33_kernel(__global const uchar *in,__global uchar* out){
    int gid=get_global_id(0);
    const uchar *d=in+gid*64;
    uint w[64];
    for(int i=0;i<16;i++){
        w[i]=((uint)d[i*4]<<24)|((uint)d[i*4+1]<<16)|((uint)d[i*4+2]<<8)|((uint)d[i*4+3]);
    }
    for(int i=16;i<64;i++)
        w[i]=w[i-16]+sig0(w[i-15])+w[i-7]+sig1(w[i-2]);
    uint a=0x6a09e667,b=0xbb67ae85,c=0x3c6ef372,d0=0xa54ff53a;
    uint e=0x510e527f,f=0x9b05688c,g=0x1f83d9ab,h=0x5be0cd19;
    for(int i=0;i<64;i++){
        uint t1=h+Sig1(e)+Ch(e,f,g)+K[i]+w[i];
        uint t2=Sig0(a)+Maj(a,b,c);
        h=g;g=f;f=e;e=d0+t1;d0=c;c=b;b=a;a=t1+t2;
    }
    uint s0=0x6a09e667+a; uint s1=0xbb67ae85+b; uint s2=0x3c6ef372+c; uint s3=0xa54ff53a+d0;
    uint s4=0x510e527f+e; uint s5=0x9b05688c+f; uint s6=0x1f83d9ab+g; uint s7=0x5be0cd19+h;
    uchar *o=out+gid*32;
    o[0]=s0>>24; o[1]=s0>>16; o[2]=s0>>8; o[3]=s0;
    o[4]=s1>>24; o[5]=s1>>16; o[6]=s1>>8; o[7]=s1;
    o[8]=s2>>24; o[9]=s2>>16; o[10]=s2>>8; o[11]=s2;
    o[12]=s3>>24; o[13]=s3>>16; o[14]=s3>>8; o[15]=s3;
    o[16]=s4>>24; o[17]=s4>>16; o[18]=s4>>8; o[19]=s4;
    o[20]=s5>>24; o[21]=s5>>16; o[22]=s5>>8; o[23]=s5;
    o[24]=s6>>24; o[25]=s6>>16; o[26]=s6>>8; o[27]=s6;
    o[28]=s7>>24; o[29]=s7>>16; o[30]=s7>>8; o[31]=s7;
}

uint rol(uint x,uint n){return (x<<n)|(x>>(32-n));}
#define F1(x,y,z) ((x) ^ (y) ^ (z))
#define F2(x,y,z) (((x)&(y)) | (~(x)&(z)))
#define F3(x,y,z) (((x)|(~(y))) ^ (z))
#define F4(x,y,z) (((x)&(z)) | (~(z)&(y)))
#define F5(x,y,z) ((x) ^ ((y)|(~(z))))
__constant uint r1[80]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,7,4,13,1,10,6,15,3,12,0,9,5,2,14,11,8,3,10,14,4,9,15,8,1,2,7,0,6,13,11,5,12,1,9,11,10,0,8,12,4,13,3,7,15,14,5,6,2,4,0,5,9,7,12,2,10,14,1,3,8,11,6,15,13};
__constant uint r2[80]={5,14,7,0,9,2,11,4,13,6,15,8,1,10,3,12,6,11,3,7,0,13,5,10,14,15,8,12,4,9,1,2,15,5,1,3,7,14,6,9,11,8,12,2,10,0,4,13,8,6,4,1,3,11,15,0,5,12,2,13,9,7,10,14,12,15,10,4,1,5,8,7,6,2,13,14,0,3,9,11};
__constant uint s1[80]={11,14,15,12,5,8,7,9,11,13,14,15,6,7,9,8,7,6,8,13,11,9,7,15,7,12,15,9,11,7,13,12,11,13,6,7,14,9,13,15,14,8,13,6,5,12,7,5,11,12,14,15,14,15,9,8,9,14,5,6,8,6,5,12,9,15,5,11,6,8,13,12,5,12,13,14,11,8,5,6};
__constant uint s2[80]={8,9,9,11,13,15,15,5,7,7,8,11,14,14,12,6,9,13,15,7,12,8,9,11,7,7,12,7,6,15,13,11,9,7,15,11,8,6,6,14,12,13,5,14,13,13,7,5,15,5,8,11,14,14,6,14,6,9,12,9,12,5,15,8,8,5,12,9,12,5,14,6,8,13,6,5,15,13,11,11};

__kernel void ripemd160_32_kernel(__global const uchar *in,__global uchar *out){
    int gid=get_global_id(0);
    const uchar *d=in+gid*64;
    uint w[16];
    for(int i=0;i<16;i++)
        w[i]=((uint)d[i*4])|((uint)d[i*4+1]<<8)|((uint)d[i*4+2]<<16)|((uint)d[i*4+3]<<24);
    uint h0=0x67452301u,h1=0xEFCDAB89u,h2=0x98BADCFEu,h3=0x10325476u,h4=0xC3D2E1F0u;
    uint a1=h0,b1=h1,c1=h2,d1=h3,e1=h4;
    uint a2=h0,b2=h1,c2=h2,d2=h3,e2=h4;
    for(int i=0;i<80;i++){
        uint f,k,t;
        if(i<16){f=F1(b1,c1,d1);k=0x00000000u;}else if(i<32){f=F2(b1,c1,d1);k=0x5A827999u;}else if(i<48){f=F3(b1,c1,d1);k=0x6ED9EBA1u;}else if(i<64){f=F4(b1,c1,d1);k=0x8F1BBCDCu;}else{f=F5(b1,c1,d1);k=0xA953FD4Eu;}
        t=rol(a1+f+w[r1[i]]+k,s1[i])+e1; a1=e1; e1=d1; d1=rol(c1,10); c1=b1; b1=t;
        if(i<16){f=F5(b2,c2,d2);k=0x50A28BE6u;}else if(i<32){f=F4(b2,c2,d2);k=0x5C4DD124u;}else if(i<48){f=F3(b2,c2,d2);k=0x6D703EF3u;}else if(i<64){f=F2(b2,c2,d2);k=0x7A6D76E9u;}else{f=F1(b2,c2,d2);k=0x00000000u;}
        t=rol(a2+f+w[r2[i]]+k,s2[i])+e2; a2=e2; e2=d2; d2=rol(c2,10); c2=b2; b2=t;
    }
    uint t=h0; h0=h1+c1+d2; h1=h2+d1+e2; h2=h3+e1+a2; h3=h4+a1+b2; h4=t+b1+c2;
    uchar *o=out+gid*20;
    o[0]=h0; o[1]=h0>>8; o[2]=h0>>16; o[3]=h0>>24;
    o[4]=h1; o[5]=h1>>8; o[6]=h1>>16; o[7]=h1>>24;
    o[8]=h2; o[9]=h2>>8; o[10]=h2>>16; o[11]=h2>>24;
    o[12]=h3; o[13]=h3>>8; o[14]=h3>>16; o[15]=h3>>24;
    o[16]=h4; o[17]=h4>>8; o[18]=h4>>16; o[19]=h4>>24;
}
