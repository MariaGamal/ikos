int main() {
    int j;
    int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int i = 1; i < 10; i+= 2) {
        j = i + 2;
        arr[i] = arr[j]; 

        j = (i+1) + 2;
        arr[i + 1] = arr[j];
    }
    return 0;
}


/*
    * Expected output:
    *
    * j = 0
    * arr[1] = arr[0]
    * j = 1
    * arr[2] = arr[1]
    
    * j = 3
    * arr[4] = arr[3]
    * j = 4
    * arr[5] = arr[4]
    
    * j = 6
    * arr[7] = arr[6]
    * j = 7
    * arr[8] = arr[7]
    
    * j = 9
    * arr[10] = arr[9]
    */

