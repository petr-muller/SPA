int a(int *a){
    (*a)++;
    return *a;
}

int b(int *b){
    (*b) *= 2;
    return *b;
}

int f(int a, int b){
    return a+b;
}

int main(void){
    int i, j = 1;
    int *k, *l;
    k = &i;
    l = &i;
    j = f(a(k), b(l)); // !!! (A) -- this is NOT a "comma" OPERATOR
    return j;
}
