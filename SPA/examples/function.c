int f(int *p){
  (*p)++;
  return *p;
}

int main(void){
  int a = 0;
  a = f(&a);
}
