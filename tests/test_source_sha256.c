/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_source_sha256.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void check(const uint8_t *data,size_t n,const char *expected)
{
    uint8_t digest[32];
    static const char hex[]="0123456789abcdef";
    char actual[65];
    chaon_source_sha256(data,n,digest);
    for(unsigned i=0;i<32;i++){actual[i*2]=hex[digest[i]>>4];actual[i*2+1]=hex[digest[i]&15];}
    actual[64]=0;
    assert(strcmp(actual,expected)==0);
}
int main(void)
{
    check((const uint8_t *)"",0,
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    check((const uint8_t *)"abc",3,
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    uint8_t source[2047];
    memset(source,'a',sizeof source);
    check(source,55,"9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318");
    check(source,56,"b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a");
    check(source,64,"ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb");
    check(source,1000,"41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
    check(source,sizeof source,"5d8098602265204dabadbc3cc14431446432926ca242f6f5699b106c51c40725");
    puts("ChaoN raw-source SHA-256 vectors: PASS");
    return 0;
}
