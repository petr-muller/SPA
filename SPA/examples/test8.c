int main(void){
  int i=0, j=1;
  *(0==j ? &i : &j) = i++;
}
