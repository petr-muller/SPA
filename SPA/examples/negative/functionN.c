int f(int *p){
  return (*p)+1;
}

int main(void){
  int a = 0;
  a = f(&a);
}
