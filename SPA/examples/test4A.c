int main(void){
    char arr[42];
    int i = 0;
    int *j = &i;
    arr[i++] = *j; // !!! (A)
    return arr[i-1];
}
