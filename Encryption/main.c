#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MASK (1<<8) - 1;

typedef struct {
    double blue, green, red;
} pixelRGB;

typedef struct {
    unsigned int height, width, padding;
    unsigned char header[54];
    unsigned char *pixel;
} imageData;

uint32_t Xorshift32(uint32_t state[static 1]) {
    uint32_t x = state[0];
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state[0] = x;
    return x;
}

void FindHeader(FILE *in, imageData *v) {
    int i;
    for(i = 0; i < 54; i ++) {
        fread(&(*v).header[i], 1, 1, in);
    }

    fseek(in, 18, SEEK_SET);
    fread(&(*v).width, sizeof(unsigned int), 1, in);
    fread(&(*v).height, sizeof(unsigned int), 1, in);
}

void FindPadding(imageData *v) {
    if((*v).width % 4 != 0)
        (*v).padding = 4 - (3 * (*v).width) % 4;
    else
        (*v).padding = 0;
}

void FindPixels(FILE *in, imageData *v) {
    (*v).pixel = (unsigned char*) malloc(3 * (*v).height * (*v).width * sizeof(unsigned char));

    fseek(in, 54, SEEK_SET);
    int i, j, poz = 0;

    for(i = 1; i <= (*v).height; i ++) {
        fseek(in, (- 1) * i * (3 * (*v).width + (*v).padding),SEEK_END);
        for(j = 0; j < 3 * (*v).width; j ++, poz ++) {
            fread(&(*v).pixel[poz], 1, 1, in);
        }

        fseek(in, (*v).padding, SEEK_CUR);
    }
}

imageData LoadImage(char *ImagePath) {
    FILE *in = fopen(ImagePath, "rb");
    imageData v;

    FindHeader(in, &v);
    FindPadding(&v);
    FindPixels(in, &v);

    fclose(in);
    return v;
}

void SaveHeader(FILE *out, imageData v) {
    int i;
    for(i = 0; i < 54; i ++) {
        fwrite(&v.header[i], 1, 1, out);
    }
}

void SavePixels(FILE *out, imageData v) {
    int i, j, poz = 0;
    unsigned char c = 0;
    for (i = v.height - 1; i >= 0; i --) {
        poz = 3 * i * v.width;
        for (j = 0; j < 3 * v.width; j ++, poz ++) {
            fwrite(&v.pixel[poz], 1, 1, out);
        }

        for(j = 0; j < v.padding; j ++) {
            fwrite(&c, 1, 1, out);
        }
    }
}

void SaveImage(imageData v, char *NewImagePath) {
    FILE *out = fopen(NewImagePath, "wb");

    SaveHeader(out, v);
    SavePixels(out, v);

    fclose(out);
}

uint32_t *CallXorshift32(uint32_t r0, int length) {
    uint32_t x[2];
    x[0] = r0;
    uint32_t *r = malloc(length * sizeof(uint32_t));
    int i;
    for(i = 1; i < length; i ++) {
        r[i] = Xorshift32(x);
    }
    return r;
}

int *DurstenfeldAlgorithm(uint32_t const *r, int n) {
    int i, j = 1, k, aux;
    int *p = malloc(n * sizeof(int));
    for (i = 0; i < n; i ++) {
        p[i] = i;
    }

    for (i = n - 1; i >= 1; i --, j ++) {
        k = r[j] % (i + 1);
        aux = p[i];
        p[i] = p[k];
        p[k] = aux;
    }

    return p;
}

int *Reverse(int const *p, int n) {
    int *pp = malloc(n * sizeof(int));
    int i;
    for(i = 0; i < n; i ++) {
        pp[p[i]] = i;
    }
    return pp;
}

unsigned char *Permute(unsigned char const *v, int const *p, int n) {
    unsigned char *pp = malloc(3 * n * sizeof(char));
    int i;
    for(i = 0; i < n; i ++) {
        pp[3 * p[i]] = v[3 * i];
        pp[3 * p[i] + 1] = v[3 * i + 1];
        pp[3 * p[i] + 2] = v[3 * i + 2];
    }
    return pp;
}

void InitialiseSV(pixelRGB *x, unsigned int sv) {
    (*x).blue = sv & MASK;
    sv >>= 8;
    (*x).green = sv & MASK;
    sv >>= 8;
    (*x).red = sv & MASK;
}

void XorPixels(unsigned char **v, unsigned char const *p, uint32_t const *r, pixelRGB x, int poz, int n) {
    uint32_t rn = r[n];
    (*v)[poz] = (unsigned char) x.blue ^ p[poz] ^ (unsigned char) rn;
    rn >>= 8;
    (*v)[poz + 1] = (unsigned char) x.green ^ p[poz + 1] ^ (unsigned char) rn;
    rn >>= 8;
    (*v)[poz + 2] = (unsigned char) x.red ^ p[poz + 2] ^ (unsigned char) rn;
}

unsigned char *CipheredImage(uint32_t sv, unsigned char const *p, uint32_t const *r, int n) {
    unsigned char *v = malloc(3 * n * sizeof(unsigned char));
    int i, poz = 0;
    pixelRGB x;

    InitialiseSV(&x, sv);
    XorPixels(&v, p, r, x, poz, n);

    for(i = 1; i < n; i ++) {
        x.blue = v[poz];
        x.green = v[poz + 1];
        x.red = v[poz + 2];

        poz += 3;
        XorPixels(&v, p, r, x, poz, n + i);
    }
    return v;
}

void Encrypt(imageData *v, uint32_t r0, uint32_t sv) {
    unsigned char *pp;
    int *p;
    uint32_t *r, n;

    n = (*v).width * (*v).height;
    r = CallXorshift32(r0, 2 * n);
    p = DurstenfeldAlgorithm(r, n);
    pp = Permute((*v).pixel, p, n);
    (*v).pixel = CipheredImage(sv, pp, r, n);

    free(r);
    free(p);
    free(pp);
}

void CallEncrypt(char *originalImagePath, char *encryptedImagePath, char *SecretKeyPath) {
    uint32_t r0, sv;

    FILE *in = fopen(SecretKeyPath, "r");
    fscanf(in, "%u %u", &r0, &sv);
    fclose(in);

    imageData v = LoadImage(originalImagePath);
    Encrypt(&v, r0, sv);
    SaveImage(v, encryptedImagePath);

    free(v.pixel);
}

unsigned char *DecipheredImage(uint32_t sv, unsigned char const *p, uint32_t const *r, int n) {
    unsigned char *v = malloc(3 * n * sizeof(unsigned char));
    int i, poz = 0;
    pixelRGB x;

    InitialiseSV(&x, sv);
    XorPixels(&v, p, r, x, poz, n);

    for(i = 1; i < n; i ++) {
        x.blue = p[poz];
        x.green = p[poz + 1];
        x.red = p[poz + 2];

        poz += 3;
        XorPixels(&v, p, r, x, poz, n + i);
    }

    return v;
}

void Decrypt(imageData *v, uint32_t r0, uint32_t sv) {
    unsigned char *w;
    int *p, *pp;
    uint32_t *r, n;

    n = (*v).width * (*v).height;
    r = CallXorshift32(r0, 2 * n);
    p = DurstenfeldAlgorithm(r, n);
    pp = Reverse(p, n);
    w = DecipheredImage(sv, (*v).pixel, r, n);
    (*v).pixel = Permute(w, pp, n);

    free(r);
    free(p);
    free(pp);
    free(w);
}

void CallDecrypt(char *encryptedImagePath, char *decryptedImagePath, char *SecretKeyPath) {
    uint32_t r0, sv;

    FILE *in = fopen(SecretKeyPath, "r");
    fscanf(in, "%u %u", &r0, &sv);
    fclose(in);

    imageData v = LoadImage(encryptedImagePath);
    Decrypt(&v, r0, sv);
    SaveImage(v, decryptedImagePath);

    free(v.pixel);
}

void InitialisePixels(pixelRGB f[], pixelRGB *x) {
    int i;
    for(i = 0; i < 256; i ++) {
        f[i].blue = 0;
        f[i].green = 0;
        f[i].red = 0;
    }

    (*x).blue = 0;
    (*x).green = 0;
    (*x).red = 0;
}

void CalcFq(unsigned char const *v, pixelRGB f[], int n) {
    int i, poz = 0;
    for(i = 0; i < n; i ++, poz +=3) {
        f[v[poz]].blue ++;
        f[v[poz + 1]].green ++;
        f[v[poz + 2]].red ++;
    }
}

void CalcChi(pixelRGB f[], pixelRGB *x, double f_med) {
    int i;
    for(i = 0; i < 256; i ++) {
        (*x).blue += ((f[i].blue - f_med) * (f[i].blue - f_med) / f_med);
        (*x).green += ((f[i].green - f_med) * (f[i].green - f_med) / f_med);
        (*x).red += ((f[i].red - f_med) * (f[i].red - f_med) / f_med);
    }
}

void ChiSquaredTest(char *imagePath) {
    pixelRGB f[256], x;
    imageData v = LoadImage(imagePath);
    double t = 256, f_med = (double) v.width * (double) v.height / t;

    InitialisePixels(f, &x);
    CalcFq(v.pixel, f, v.width * v.height);
    CalcChi(f, &x, f_med);

    printf("Chi-squared test on RGB channels for %s:\nR:%.2lf\nG:%.2lf\nB:%.2lf\n", imagePath, x.red, x.green, x.blue);

    free(v.pixel);
}

void TaskI(char *imagePath) {
    char secretKeyPath[101], encryptedImagePath[101];

    printf("Numele fisierului care contine imaginea initiala: ");
    fgets(imagePath, 101, stdin);   imagePath[strlen(imagePath) - 1] = '\0';

    printf("Numele fisierului care va contine imaginea criptata : ");
    fgets(encryptedImagePath, 101, stdin);  encryptedImagePath[strlen(encryptedImagePath) - 1] = '\0';

    printf("Numele fisierului care contine cheia secreta : ");
    fgets(secretKeyPath, 101, stdin);   secretKeyPath[strlen(secretKeyPath) - 1] = '\0';

    CallEncrypt(imagePath, encryptedImagePath, secretKeyPath);
}

void TaskII(char *encryptedImagePath) {
    char secretKeyPath[101], decryptedImagePath[101];

    printf("Numele fisierului care contine imaginea criptata : ");
    fgets(encryptedImagePath, 101, stdin);  encryptedImagePath[strlen(encryptedImagePath) - 1] = '\0';

    printf("Numele fisierului in care va contine imaginea decriptata : ");
    fgets(decryptedImagePath, 101, stdin);  decryptedImagePath[strlen(decryptedImagePath) - 1] = '\0';

    printf("Numele fisierului care contine cheia secreta : ");
    fgets(secretKeyPath, 101, stdin);   secretKeyPath[strlen(secretKeyPath) - 1] = '\0';

    CallDecrypt(encryptedImagePath, decryptedImagePath, secretKeyPath);
}

void TaskIII(char *imagePath, char *encryptedImagePath) {
    ChiSquaredTest(imagePath);
    ChiSquaredTest(encryptedImagePath);
}

int main() {
    char imagePath[101], encryptedImagePath[101];
    TaskI(imagePath);
    TaskII(encryptedImagePath);
    TaskIII(imagePath, encryptedImagePath);
    return 0;
}
