int a(int *a){
    return 0;
}

int b(int *b){
    return 0;
}

int f(int a, int b){
    return a+b;
}

int main(void){
    int i, j = 1;
    j = f(a(&i), b(&i)); // !!! (A) -- this is NOT a "comma" OPERATOR
    return j;
}
