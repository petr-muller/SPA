int f(int *p, int *q){
  (*p)++;
  return 42;
}

int main(void){
  int a = 0;
  a = f(&a,&a);
}
