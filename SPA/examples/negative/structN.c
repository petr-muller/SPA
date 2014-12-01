typedef struct test{
  int a;
  int b;
} test;

int main(void){
  test x;
  x.a = 1;
  x.b = 2;
  x.a = x.b++;
}
