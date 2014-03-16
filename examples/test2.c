#define MACRO i = (*j)++; // !!!(A) -- assignment order is undefined

int main(){
	int i = 0;
	int *j = &i;
    MACRO
    return i;
}
