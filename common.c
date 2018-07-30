int exists(const char *val, const char *arr) {
    for (int i = 0; arr[i] != '\0'; i++) {
        if (arr[i] == *val)
            return 1;
    }
    return 0;
}

char *str_dup(const char *source) {
    char *cpy = malloc(strlen(source) + 1);
    return strcpy(cpy, source);
}

char *str_dup_n(const char *source, size_t len) {
    char *cpy = malloc(len + 1);
    strncpy(cpy, source, len);
    cpy[len] = '\0';
    return cpy;
}
