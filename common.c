int exists(const char *val, const char *arr) {
    for (int i = 0; arr[i] != '\0'; i++) {
        if (arr[i] == *val)
            return 1;
    }
    return 0;
}

char *str_dup(const char *source) {
    char *cpy = calloc(1, strlen(source) + 1);
    return strcpy(cpy, source);
}

char *str_dup_n(const char *source, size_t len) {
    char *cpy = calloc(1, len + 1);
    strncpy(cpy, source, len);
    cpy[len] = '\0';
    return cpy;
}

char *load_file(const char *name) {
    FILE *infile;
    char *buffer;
    long size;

    infile = fopen(name, "r");

    if (infile == NULL)
        return NULL;

    fseek(infile, 0L, SEEK_END);
    size = ftell(infile);
    fseek(infile, 0L, SEEK_SET);
    buffer = (char *) calloc((size_t) size, sizeof(char));

    if (buffer == NULL)
        return NULL;

    fread(buffer, sizeof(char), (size_t) size, infile);

    fclose(infile);

    return buffer;
}