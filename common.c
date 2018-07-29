int exists(const char *val, const char *arr) {
    for (int i = 0; arr[i] != '\0'; i++) {
        if (arr[i] == *val)
            return 1;
    }
    return 0;
}