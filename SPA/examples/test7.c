int main(void){
  int i = 0;
  int j;
  j = 0==i ? i++ + ++i : ++i;
  return j;
}
