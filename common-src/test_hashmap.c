#include "hashmap.h"
#include <stdio.h>

int main() {
    struct hashmap * hash = hash_new(9);
    char name1[GLOBALNAME_LENGTH] = "hello";
    char name2[GLOBALNAME_LENGTH] = "hanxi";
    char name3[GLOBALNAME_LENGTH] = "hanxi1";
    char name4[GLOBALNAME_LENGTH] = "hanxi2";
    hash_insert(hash, name1,1);
    hash_insert(hash, name2,2);
    hash_insert(hash, name3,3);
    hash_insert(hash, name4,4);
    int v1 = hash_search(hash, name1);
    int v2 = hash_search(hash, name2);
    int v3 = hash_search(hash, name3);
    int v4 = hash_search(hash, name4);
    printf("v1=%d,v2=%d,v3=%d,v4=%d\n",v1,v2,v3,v4);
    hash_remove(hash, name1);
    v1 = hash_search(hash, name1);
    v2 = hash_search(hash, name2);
    v3 = hash_search(hash, name3);
    v4 = hash_search(hash, name4);
    printf("v1=%d,v2=%d,v3=%d,v4=%d\n",v1,v2,v3,v4);
    hash_insert(hash, name1,1);
    hash_remove(hash, name3);
    v1 = hash_search(hash, name1);
    v2 = hash_search(hash, name2);
    v3 = hash_search(hash, name3);
    v4 = hash_search(hash, name4);
    printf("v1=%d,v2=%d,v3=%d,v4=%d\n",v1,v2,v3,v4);
    int i=0;
    for (i=0; i<1000; i++) {
        name1[5] = (char)i;
        hash_insert(hash, name1,i);
    }
    name1[5] = '\0';
    v1 = hash_search(hash, name1);
    v2 = hash_search(hash, name2);
    v3 = hash_search(hash, name3);
    v4 = hash_search(hash, name4);
    printf("v1=%d,%s,v2=%d,v3=%d,v4=%d\n",v1,name1,v2,v3,v4);
    printf("size=%d,%d\n",sizeof(struct keyvalue *),sizeof(struct keyvalue));
    hash_delete(hash);
}
