int main(void){
        int j = 42;
        int k = 0;
        int *i = &k;
        j = (*i)++;
        return j;
}
