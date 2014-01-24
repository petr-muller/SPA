int main(void){
    int i = 2;
    int *j = &i;
    int a = 3;
    int *b = &a;
    *(b!=0?&i:&i) = 42;
    return *(j);
}
