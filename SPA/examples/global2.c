int a = 0;

int f(int *b){
  return (*b)++;
}

int main(void){
  a = f(&a);
}
