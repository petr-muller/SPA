int main(){
    int i, *j, *k;
    j=&i;
    k=&i;
    return i + *j + *k;
}

