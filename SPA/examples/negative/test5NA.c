int main(void){
  int i = 0, j = 0;
  int *k = &i;
  j = i++ && ++(*k);
  return j;
}
