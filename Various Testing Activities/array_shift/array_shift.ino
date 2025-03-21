int array[] = {1, 2, 3, 4, 5};
byte decimalLocation = 1;
float length = 1.003;
int shift = 1234;
int x = 0;
void setup() {
  Serial.begin(9600);
   while(!Serial);
  // for(int i=0; i<5;i++){
  //   shift+=pow(10,i);
  // }
  Serial.println(shift);
  // put your setup code here, to run once:
  // for (int i=0;i<5;i++){
  //     length += array[i]*(pow(10,(decimalLocation - i)));
  //   }
  // Serial.print(length,decimalLocation);
  // Serial.println();
  // for(int i=0; i < 5; i++){
  // Serial.print(array[i]);
  // }
  // Serial.println();
  // // array[0] = array[1];
  // // array[1] = array[2];
  // // ...
  // // array[3] == array[4];
  // while(!array[0]){
  //   for(int shift = 0; shift < 5; shift++){
  //   array[shift] = array[shift+1];
  //   }
  //   x++;
  //   if(x>200){
  //     break;
  //   }
  // }
  // for(int i=0; i < 5; i++){
  // Serial.print(array[0]);
  // }
  // Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:

}
