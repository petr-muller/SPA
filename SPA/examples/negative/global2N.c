int a = 0;

int f(int *b){
  return a++;
}

int main(void){
  int a;
  a = f(&a);
}
