int main(void){
  int i = 0;
  int *j = &i;
  int k;
  k = 0==i ? i++ : ++(*j);
  return k;
}
