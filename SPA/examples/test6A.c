int main(void){
    int k;
    int i = 0;
    int *j = &i;
    k = i++ + ++(*j); // !!! 
    return k;
}

