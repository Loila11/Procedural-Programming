#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define x_size 11
#define y_size 15

typedef struct {
    double blue, green, red;
} pixelRGB;

typedef struct {
    unsigned int height, width, padding;
    unsigned char header[54];
    unsigned char *pixel;
} imageData;

typedef struct {
    unsigned int x, y;
    double corr;
    pixelRGB c;
} window;

typedef struct {
    imageData v;
    window f;
    double med, dev;
} corrData;

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

void InitialiseColors(pixelRGB c[]) {
    c[0].red = 255;    c[0].green = 0;      c[0].blue = 0;      // rosu
    c[1].red = 255;    c[1].green = 255;    c[1].blue = 0;      // galben
    c[2].red = 0;      c[2].green = 255;    c[2].blue = 0;      //verde
    c[3].red = 0;      c[3].green = 255;    c[3].blue = 255;    // cyan
    c[4].red = 255;    c[4].green = 0;      c[4].blue = 255;    // magenta
    c[5].red = 0;      c[5].green = 0;      c[5].blue = 255;    // albastru
    c[6].red = 192;    c[6].green = 192;    c[6].blue = 192;    // argintiu
    c[7].red = 255;    c[7].green = 140;    c[7].blue = 0;      // portocaliu
    c[8].red = 128;    c[8].green = 0;      c[8].blue = 128;    // mov
    c[9].red = 128;    c[9].green = 0;      c[9].blue = 0;      // maro
}

int cmp(const void *a, const void *b) {
    if(((window *)a)->corr > ((window *)b)->corr)
        return -1;
    if(((window *)a)->corr < ((window *)b)->corr)
        return 1;
    return 0;
}

unsigned int CalcStartPoz(imageData v, window f) {
    unsigned int x_start = x_size / 2;
    unsigned int y_start = y_size / 2;
    unsigned int poz = 3 * (f.y - y_start) * v.width + 3 * (f.x - x_start);
    return poz;
}

void HorizontalDraw(imageData *v, int poz, pixelRGB c) {
    int i;
    for(i = 0; i < x_size; i ++, poz += 3) {
        (*v).pixel[poz] = (unsigned char) c.blue;
        (*v).pixel[poz + 1] = (unsigned char) c.green;
        (*v).pixel[poz + 2] = (unsigned char) c.red;
    }
}

void VerticalDraw(imageData *v, int poz, pixelRGB c) {
    int i;
    for(i = 0; i < y_size; i ++, poz += 3 * (*v).width) {
        (*v).pixel[poz] = (unsigned char) c.blue;
        (*v).pixel[poz + 1] = (unsigned char) c.green;
        (*v).pixel[poz + 2] = (unsigned char) c.red;
    }
}

void PerimeterDraw(char *imagePath, window f, pixelRGB c) {
    imageData v = LoadImage(imagePath);
    unsigned int poz = CalcStartPoz(v, f);

    HorizontalDraw(&v, poz, c);
    HorizontalDraw(&v, poz + 3 * (y_size - 1) * v.width, c);

    VerticalDraw(&v, poz, c);
    VerticalDraw(&v, poz + 3 * (x_size - 1), c);

    SaveImage(v, imagePath);
    free(v.pixel);
}

void Grayscale(char *imagePath) {
    imageData v = LoadImage(imagePath);
    int i, j, poz = 0;
    unsigned char aux;

    for(i = 0; i < v.height; i ++) {
        for(j = 0; j < v.width; j ++, poz += 3) {
            aux = (unsigned char) (0.299 * v.pixel[poz + 2] + 0.587 * v.pixel[poz + 1] + 0.114 * v.pixel[poz]);
            v.pixel[poz] = v.pixel[poz + 1] = v.pixel[poz + 2] = aux;
        }
    }

    SaveImage(v, imagePath);
    free(v.pixel);
}

double CalcMed(imageData v, window f) {
    unsigned int poz = CalcStartPoz(v, f);
    double s_med = 0;
    int i, j;

    for(i = 0; i < y_size; i ++, poz += 3 * v.width) {
        for(j = 0; j < x_size; j ++) {
            s_med += v.pixel[poz + 3 * j];
        }
    }

    s_med /= (x_size * y_size);
    return s_med;
}

double StandardDeviation(imageData v, window f, double med) {
    unsigned int poz = CalcStartPoz(v, f);
    double dev = 0;
    int i, j;

    for(i = 0; i < y_size; i ++, poz += 3 * v.width) {
        for(j = 0; j < x_size; j ++) {
            dev += (v.pixel[poz + 3 * j] - med) * (v.pixel[poz + 3 * j] - med);
        }
    }

    dev /= (x_size * y_size - 1);
    dev = sqrt(dev);
    return dev;
}

double CalcCorrSum(corrData image, corrData template) {
    int pozImage = CalcStartPoz(image.v, image.f);
    int pozTemplate = CalcStartPoz(template.v, template.f);

    double corr = 0;
    int i, j, intensityImage, intensityTemplate;

    for(i = 0; i < y_size; i ++, pozImage += 3 * image.v.width, pozTemplate += 3 * template.v.width) {
        for(j = 0; j < x_size; j ++) {
            intensityTemplate = template.v.pixel[pozTemplate + 3 * j];
            intensityImage = image.v.pixel[pozImage + 3 * j];
            corr += ((intensityImage - image.med) * (intensityTemplate - template.med) / (image.dev * template.dev));
        }
    }

    return corr;
}

double CrossCorrelation(corrData image, corrData template) {
    image.med = CalcMed(image.v, image.f);
    template.med = CalcMed(template.v, template.f);

    image.dev = StandardDeviation(image.v, image.f, image.med);
    template.dev = StandardDeviation(template.v, template.f, template.med);

    double corr = CalcCorrSum(image, template);
    corr /= (x_size * y_size);

    return corr;
}

void ImageSlide(corrData image, corrData template, double ps, unsigned int *ct, window **D) {
    double corr = 0;

    for(image.f.y = template.f.y; image.f.y < image.v.height - template.f.y; image.f.y ++) {
        for(image.f.x = template.f.x; image.f.x < image.v.width - template.f.x; image.f.x ++) {
            corr = CrossCorrelation(image, template);

            if(corr > ps) {
                (*D) = realloc ((*D), ((*ct) + 1) * sizeof (window));
                (*D)[(*ct)] = image.f;
                (*D)[(*ct)].corr = corr;
                (*D)[(*ct)].c = template.f.c;
                (*ct) ++;
            }
        }
    }
}

void TemplateMatching(char *imagePath, char *templatePath, double ps, unsigned int *ct, window **D, pixelRGB c) {
    Grayscale(imagePath);
    Grayscale(templatePath);

    corrData image, template;
    image.v = LoadImage(imagePath);
    template.v = LoadImage(templatePath);

    template.f.x = x_size / 2;
    template.f.y = y_size / 2;
    template.f.c = c;

    ImageSlide(image, template, ps, &(*ct), &(*D));

    free(image.v.pixel);
    free(template.v.pixel);
}

void UpperRightCorner(window f, int *x, int *y) {
    (*x) = f.x + x_size / 2;
    (*y) = f.y + y_size / 2;
}

void BottomLeftCorner(window f, int *x, int *y) {
    (*x) = f.x - x_size / 2;
    (*y) = f.y - y_size / 2;
}

void IntersectionUpperRightCorner(window f1, window f2, int *x, int *y) {
    int x1, x2, y1, y2;

    UpperRightCorner(f1, &x1, &y1);
    UpperRightCorner(f2, &x2, &y2);

    (*x) = min(x1, x2);
    (*y) = min(y1, y2);
}

void IntersectionBottomLeftCorner(window f1, window f2, int *x, int *y) {
    int x1, x2, y1, y2;

    BottomLeftCorner(f1, &x1, &y1);
    BottomLeftCorner(f2, &x2, &y2);

    (*x) = max(x1, x2);
    (*y) = max(y1, y2);
}

double IntersectionArea(window *f, int x, int y) {
    if(abs((int)(f[x].x - f[y].x)) > x_size || abs((int)(f[x].y - f[y].y) > y_size))
        return 0;

    int x1, x2;
    int y1, y2;

    IntersectionBottomLeftCorner(f[x], f[y], &x1, &y1);
    IntersectionUpperRightCorner(f[x], f[y], &x2, &y2);

    double xy_area = (x2 - x1 + 1) * (y2 - y1 + 1);
    return xy_area;
}

double SpatialOverlap(window *f, int x, int y) {
    double x_area, y_area, xy_area, overlap;

    x_area = y_area = x_size * y_size;
    xy_area = IntersectionArea(f, x, y);
    overlap = xy_area / (x_area + y_area - xy_area);

    return overlap;
}

void ItemsRemoval(window **f, _Bool const a[], unsigned int *n) {
    unsigned int i, ct = 0;
    for(i = 0; i < (*n); i ++)
        if(a[i] == 1)
            ct ++;

    window *d = malloc(ct * sizeof(window));
    ct = 0;
    for(i = 0; i < (*n); i ++) {
        if (a[i] == 1) {
            d[ct] = (*f)[i];
            ct ++;
        }
    }

    //free(f);
    (*n) = ct;
    (*f) = d;
}

void NonMaxRemoval(window **f, unsigned int *n) {
    int i, j;
    _Bool a[(*n)];
    double ps = 0.2;

    // initializez vectorul de aparitii
    for(i = 0; i < (*n); i ++)
        a[i] = 1;

    for(i = 0; i < (*n); i ++)
        if (a[i] == 1)  // daca nu a fost deja eliminat elementul de pe aceasta pozitie
            for (j = i + 1; j < (*n); j ++)
                if (a[j] == 1 && SpatialOverlap((*f), i, j) > ps)
                    a[j] = 0;   // marchez detectiile care trebuie eliminate

    ItemsRemoval(&(*f), a, &(*n));
}

void InitialiseAux(char *imagePath, char *auxiliaryImagePath) {
    imageData v = LoadImage(imagePath);
    SaveImage(v, auxiliaryImagePath);
    free(v.pixel);
}

void TaskIV(char *imagePath, window **f, unsigned int *ct) {
    char templatePath[101], *auxiliaryImagePath = "auxiliary_image.bmp";
    double ps = 0.5;
    int i;
    pixelRGB c[10];

    printf("Numele fisierului care contine imaginea color: ");
    fgets(imagePath, 101, stdin);   imagePath[strlen(imagePath) - 1] = '\0';

    InitialiseColors(c);
    InitialiseAux(imagePath, auxiliaryImagePath);

//    printf("Numele fisierelor care contin sabloanele :\n");
//    for(i = 0; i < 10; i ++) {
//        printf("Cifra %d: ", i);
//        fgets(templatePath, 101, stdin);    templatePath[strlen(templatePath) - 1] = '\0';
//        TemplateMatching(auxiliaryImagePath, templatePath, ps, &(*ct), &(*f), c[i]);
//    }

    TemplateMatching(auxiliaryImagePath, "cifra0.bmp", ps, &(*ct), &(*f), c[0]);
    TemplateMatching(auxiliaryImagePath, "cifra1.bmp", ps, &(*ct), &(*f), c[1]);
    TemplateMatching(auxiliaryImagePath, "cifra2.bmp", ps, &(*ct), &(*f), c[2]);
    TemplateMatching(auxiliaryImagePath, "cifra3.bmp", ps, &(*ct), &(*f), c[3]);
    TemplateMatching(auxiliaryImagePath, "cifra4.bmp", ps, &(*ct), &(*f), c[4]);
    TemplateMatching(auxiliaryImagePath, "cifra5.bmp", ps, &(*ct), &(*f), c[5]);
    TemplateMatching(auxiliaryImagePath, "cifra6.bmp", ps, &(*ct), &(*f), c[6]);
    TemplateMatching(auxiliaryImagePath, "cifra7.bmp", ps, &(*ct), &(*f), c[7]);
    TemplateMatching(auxiliaryImagePath, "cifra8.bmp", ps, &(*ct), &(*f), c[8]);
    TemplateMatching(auxiliaryImagePath, "cifra9.bmp", ps, &(*ct), &(*f), c[9]);
}

void TaskV(char *imagePath, window *f, unsigned int ct) {
    qsort(f, ct, sizeof(window), cmp);
    NonMaxRemoval(&f, &ct);

    int i;
    for(i = 0; i < ct; i ++) {
        PerimeterDraw(imagePath, f[i], f[i].c);
    }
}

int main() {
    char imagePath[101];
    unsigned int ct = 0;
    window *f = malloc(sizeof(window));

    TaskIV(imagePath, &f, &ct);
    TaskV(imagePath, f, ct);

    free(f);
    return 0;
}
