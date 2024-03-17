#include<stdio.h>
#include <unistd.h>
int main(){
    while(1){
        printf("Hi, from background process2\n");
        sleep(10);
    }
}