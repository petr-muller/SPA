int main(){
	int i = 0;
	int *j = &i;
	i = (*j)++; // !!!(A) -- assignment order is undefined
    return i;
}
