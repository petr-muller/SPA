#define MACRO i = (*j)++; // !!! (A)

int main(){
  int i = 0;
  int *j = &i;
  MACRO
  return i;
}
