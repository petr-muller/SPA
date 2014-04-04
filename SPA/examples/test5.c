int main(void){
    char arr[42];
    int i = 0;
    arr[i++] = i; // !!! -- The index is always the same, however, the value stored may and may not be incremented
    return arr[i-1];
}
