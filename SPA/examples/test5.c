int main(void){
    char arr[42];
    int i = 0;
    arr[i] = i++; // !!! 
    return arr[i-1];
}

