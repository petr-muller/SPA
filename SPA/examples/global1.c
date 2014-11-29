int a = 0;

int f(void){
  return a++;
}

int main(void){
  a = f();
}
