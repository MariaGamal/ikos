int a[4];
int main(int argc, char *argv[]) {
    for (int i = 0; i < 4; i++) {
            int j = i + 1;
        a[i] = a[i+1];
    }
}