#define MACRO i = (*j)++; // !!!(A)

int main(){
	int i = 0;
	int *j = &i;
  MACRO
  int *k = &i;
  *j = (*k)++;
  return i;
}
