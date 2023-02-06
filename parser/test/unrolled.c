int main() {
  int j;
  int x = 0;
  int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // for (int i = 2; i < 5; i += 1) {
  //   if (i % 2 == 0) {
  //     j = i + 2;
  //     arr[i] = arr[j];
  //   } else {
  //     j = (i + 1) + 2;
  //     arr[i] = arr[j];
  //   }
  // }

  // for (int i = 1; i < 5; i += 1) {
  //   arr[i] = arr[i + 1];
  // }

  for (int i = 0; i < 10; i += 2) {
    j = i + 1;
    x = x + 1;
    arr[i] = arr[j];

    j = (i + 1) + 1;
    x = x + 1;
    arr[i + 1] = arr[j];
  }

  // 0 1
  // 1 2 
  // ; -%main.19.offset+%main.10.offset <= 20;

  // for (int i = 15; i < 25; i += 3) {
  //   j = i + 2;
  //   arr[i] = arr[j];

  //   j = (i + 1) + 2;
  //   arr[i + 1] = arr[j];
  // }

  return 0;
}
