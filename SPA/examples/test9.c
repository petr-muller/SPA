int *f(int *a, int *b){
    return a;
}

int main(void){
    int a = 1, b = 1;
    *(f(&a,&b)) = a++;
    return a;
}
