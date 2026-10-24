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
    puts("ChaoN raw-source SHA-256 vectors: PASS");
    return 0;
}
