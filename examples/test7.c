int main(void){
    int j;
    int i = 0;
    j = i++ + ++i; // !!! -- The sum is expectable to always be the same, however, the behavior is undefined meaning this can result in some nasty things
}

