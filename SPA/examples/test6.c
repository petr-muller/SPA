int main(void){
    char arr[42];
    int i = 0;
    arr[i] = i++; // !!! -- The value stored is always the same, however, the index may and may not be incremented before the array subscription
    return arr[i-1];
}

