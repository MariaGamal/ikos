#include <stdio.h>

int main()
{
    int arr1[3] = {1, 2, 3};
    int arr2[3] = {10, 9, 8};

    for (int i = 0; i < 10; i += 2)
    {
        arr2[i] = arr1[i] + arr2[i];
        arr2[i + 1] = arr1[i + 1] + arr2[i + 1];
    }

    printf("===============================\n");
    for (int i = 0; i < 3; i++)
    {
        printf("========== Element %d ==========\n", i);
        printf("arr1[%d] = %d\n", i, arr1[i]);
        printf("arr2[%d] = %d\n", i, arr2[i]);
    }
    printf("===============================\n");

    return 0;
}