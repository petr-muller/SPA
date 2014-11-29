int main(void){
  double a[32] = {0.0};
  
  for(int i=0; i<32; ++i){
    for(int j=0; j<32; ++j){
      a[i] = (a[j])++;
    }
  }
}
