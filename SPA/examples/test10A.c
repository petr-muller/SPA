int main(void){
  int i = 0, j = 0;;
  int *k = &i;
  k = &j;
  i = (*k)++;
  return i;
}
