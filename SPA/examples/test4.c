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
    int i = 1;
    i = f(a(&i), b(&i)); // !!!(A) this is NOT a "comma OPERATOR", therefore evaluation order of f arguments is unspecified
    return i;
}
