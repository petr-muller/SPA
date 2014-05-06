int main(void){
  int i = 42, j = 42;
  int *k = &i;
  k = &j;
  i = (*k)++;
  return i;
}
