int main(void){
	int i = 1;
	i = i++; // !!! -- assignment order is undefined
	return i;
}
