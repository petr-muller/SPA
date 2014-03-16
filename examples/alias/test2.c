#define MACRO return i + *j;

int main(){
    int i, *j;
    j = &i;
    MACRO
}

