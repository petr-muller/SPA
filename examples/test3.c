int f(int *a){
    (*a)++;
    return *a;
}

int g(int *b){
    (*b) *= 2;
    return *b;
}

int main(){
    int i = 1;
    i = f(&i) + g(&i); // !!!(A) -- function calling order is undefined
    return i;
}
