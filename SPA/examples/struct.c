typedef struct test{
  int a;
  int b;
} test;

int main(void){
  test x;
  x.a = 1;
  x.a = x.a++;
}
