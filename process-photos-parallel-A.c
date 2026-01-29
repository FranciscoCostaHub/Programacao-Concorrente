#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <gd.h>
#include "image-lib.h"

#define MAX_IMAGES 10000
#define MAX_PATH 4096

//Estrutura para informacao de cada imagem
typedef struct {
    char filename[256];
    long size;
}  image_info;

// Estrutura para passar dados a cada thread
typedef struct {
    char **image_files;           // ponteiro para array de strings (nomes dos ficheiros)
    int num_images;               // total de imagens
    int start_ind;                // onde a thread comeca
    int end_ind;                  // onde a thread termina
    char input_dir[MAX_PATH];     // Pasta de entrada
    char output_dir[MAX_PATH];    // Pasta de saida
    int thread_id;                // ID da thread
    struct timespec start_time;   // Quando comecou a trabalhar
    struct timespec end_time;     // Quando terminou o trabalho
} thread_info;


//Comparacao por nome (ordem alfabetica)
int compare_by_name(const void *a, const void *b) {
     image_info *img_a = ( image_info *)a;
     image_info *img_b = ( image_info *)b;
    return strcmp(img_a->filename, img_b->filename);
}

// Comparacao por tamanho (ordem crescente) 
int compare_by_size(const void *a, const void *b) {
     image_info *img_a = ( image_info *)a;
     image_info *img_b = ( image_info *)b;
    return (img_a->size > img_b->size) - (img_a->size < img_b->size);
}


//simples verificação para ver se o file existe
int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}


// processa a imagem aplicando as 5 transformações
void process_image(const char *input_path, const char *output_dir, const char *filename) {
    char output_path[MAX_PATH];
    gdImagePtr original, transformed;
    
    //Ler imagem original
    original = read_jpeg_file((char *)input_path);
    if (!original) {
        fprintf(stderr, "\tErro ao ler %s\n", input_path);
        return;
    }
    
    //Contrast
    snprintf(output_path, MAX_PATH, "%s/contrast_%s", output_dir, filename);
    if (!file_exists(output_path)) {
        transformed = contrast_image(original);
        if (transformed) {
            write_jpeg_file(transformed, output_path);
            gdImageDestroy(transformed);
        }
    }
    
    //BLUR
    snprintf(output_path, MAX_PATH, "%s/blur_%s", output_dir, filename);
    if (!file_exists(output_path)) {
        transformed = blur_image(original);
        if (transformed) {
            write_jpeg_file(transformed, output_path);
            gdImageDestroy(transformed);
        }
    }
    
    //SEPIA
    snprintf(output_path, MAX_PATH, "%s/sepia_%s", output_dir, filename);
    if (!file_exists(output_path)) {
        transformed = sepia_image(original);
        if (transformed) {
            write_jpeg_file(transformed, output_path);
            gdImageDestroy(transformed);
        }
    }
    
    //THUMB
    snprintf(output_path, MAX_PATH, "%s/thumb_%s", output_dir, filename);
    if (!file_exists(output_path)) {
        transformed = thumb_image(original);
        if (transformed) {
            write_jpeg_file(transformed, output_path);
            gdImageDestroy(transformed);
        }
    }
    
    //GRAY
    snprintf(output_path, MAX_PATH, "%s/gray_%s", output_dir, filename);
    if (!file_exists(output_path)) {
        transformed = gray_image(original);
        if (transformed) {
            write_jpeg_file(transformed, output_path);
            gdImageDestroy(transformed);
        }
    }
    
    //LIBERTAR IMAGEM ORIG
    gdImageDestroy(original);
}


// FUNÇÃO DE CADA THREAD WORKER
void *thread_worker(void *arg) {
    thread_info *data = (thread_info *)arg;
    
    // inicia a contagem do tempo
    clock_gettime(CLOCK_MONOTONIC, &data->start_time);
    
    // Processar imagens atribuidas a esta thread
    for (int i = data->start_ind; i < data->end_ind; i++) {
        char input_path[MAX_PATH];
        snprintf(input_path, MAX_PATH, "%s/%s", data->input_dir, data->image_files[i]);
        
        printf("Thread %d: A processar thread %s\n", data->thread_id, data->image_files[i]);
        process_image(input_path, data->output_dir, data->image_files[i]);
    }
    
    // Para a contagem de tempo
    clock_gettime(CLOCK_MONOTONIC, &data->end_time);
    
    return NULL;
}


//main
int main(int argc, char *argv[]) {
    struct timespec main_start, main_end;
    struct timespec parallel_start, parallel_end;
    
    // Iniciar contagem de tempo total
    clock_gettime(CLOCK_MONOTONIC, &main_start);
    
    // Validação dos argumentos
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <diretoria> <num_threads> <-name|-size>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s ./images 4 -size\n", argv[0]);
        exit(1);
    }
    
    char *input_dir = argv[1];
    int num_threads = atoi(argv[2]);
    char *sort_mode = argv[3];
    
    if (num_threads <= 0) {
        fprintf(stderr, "Erro: Numero de threads deve ser positivo\n");
        exit(1);
    }
    
    if (strcmp(sort_mode, "-name") != 0 && strcmp(sort_mode, "-size") != 0) {
        fprintf(stderr, "Erro: Modo de ordenacao deve ser -name ou -size\n");
        exit(1);
    }
    // Fiz isto so para mostrar as informações iniciais porcausa daquele problema
    printf("=== Process Photos Parallel A ===\n");
    printf("Diretoria: %s\n", input_dir);
    printf("Threads: %d\n", num_threads);
    printf("Ordenacao: %s\n", sort_mode);
    printf("\n");
    
    // Cria diretoria de output
    char output_dir[MAX_PATH];
    snprintf(output_dir, MAX_PATH, "Result-image-dir");
    if (create_directory(output_dir) == 0) {
        fprintf(stderr, "Erro ao criar diretoria %s\n", output_dir);
        exit(1);
    }
    
    DIR *dir = opendir(input_dir);
    if (!dir) {
        fprintf(stderr, "Erro ao abrir diretoria %s\n", input_dir);
        exit(1);
    }
    
     image_info images[MAX_IMAGES];
    int num_images = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        int len = strlen(entry->d_name);
        
        // Verificar se termina em .jpeg
        if (len > 5 && strcmp(entry->d_name + len - 5, ".jpeg") == 0) {
            // Guardar nome
            strncpy(images[num_images].filename, entry->d_name, 255);
            images[num_images].filename[255] = '\0';
            
            // Obter tamanho do ficheiro
            char full_path[MAX_PATH];
            snprintf(full_path, MAX_PATH, "%s/%s", input_dir, entry->d_name);
            struct stat st;
            if (stat(full_path, &st) == 0) {
                images[num_images].size = st.st_size;
            } else {
                images[num_images].size = 0;
            }
            
            num_images++;
            if (num_images >= MAX_IMAGES) {
                fprintf(stderr, "Aviso: Limite de %d imagens atingido\n", MAX_IMAGES);
                break;
            }
        }
    }
    closedir(dir);
    
    printf("Encontradas %d imagens .jpeg\n", num_images);
    
    if (num_images == 0) {
        printf("Nenhuma imagem para processar.\n");
        return 0;
    }
    
    // ORDENAR IMAGENS
    if (strcmp(sort_mode, "-name") == 0) {
        qsort(images, num_images, sizeof( image_info), compare_by_name);
        printf("Imagens ordenadas por nome\n\n");
    } else {
        qsort(images, num_images, sizeof( image_info), compare_by_size);
        printf("Imagens ordenadas por tamanho\n\n");
    }
    
    //Criar array de strings para passar as threads
    char **image_files = malloc(num_images * sizeof(char *));
    for (int i = 0; i < num_images; i++) {
        image_files[i] = strdup(images[i].filename);
    }
    
    //Tempo nao paralelo termina aqui
    clock_gettime(CLOCK_MONOTONIC, &parallel_start);
    
    //CRIAR E LANCAR THREADS
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    thread_info *thread_data = malloc(num_threads * sizeof(thread_info));
    
    //Dividir trabalho entre threads
    int images_per_thread = num_images / num_threads;
    int remainder = num_images % num_threads;
    
    int start_idx = 0;
    for (int t = 0; t < num_threads; t++) {
        thread_data[t].image_files = image_files;
        thread_data[t].num_images = num_images;
        thread_data[t].start_ind = start_idx;
        
        //Distribuir imagens restantes pelas primeiras threads
        int images_for_this_thread = images_per_thread + (t < remainder ? 1 : 0);
        thread_data[t].end_ind = start_idx + images_for_this_thread;
        
        //Copiar diretorias com garantia de null terminator
        strncpy(thread_data[t].input_dir, input_dir, MAX_PATH - 1);
        thread_data[t].input_dir[MAX_PATH - 1] = '\0'; 
        
        strncpy(thread_data[t].output_dir, output_dir, MAX_PATH - 1);
        thread_data[t].output_dir[MAX_PATH - 1] = '\0';
        
        thread_data[t].thread_id = t;
        
        start_idx = thread_data[t].end_ind;
        
        pthread_create(&threads[t], NULL, thread_worker, &thread_data[t]);
    }
    
    //AGUARDAR THREAD
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
    
    //Tempo paralelo termina
    clock_gettime(CLOCK_MONOTONIC, &parallel_end);
    clock_gettime(CLOCK_MONOTONIC, &main_end);
    
    //CALCULAR TEMPOS
    struct timespec total_time = diff_timespec(&main_end, &main_start);
    struct timespec parallel_time = diff_timespec(&parallel_end, &parallel_start);
    struct timespec non_parallel_time = diff_timespec(&main_end, &main_start);
    
    //tempo nao paralelo = tempo total - tempo paralelo
    non_parallel_time.tv_sec = total_time.tv_sec - parallel_time.tv_sec;
    non_parallel_time.tv_nsec = total_time.tv_nsec - parallel_time.tv_nsec;
    if (non_parallel_time.tv_nsec < 0) {
        non_parallel_time.tv_nsec += 1000000000;
        non_parallel_time.tv_sec--;
    }
    
    //MOSTRAR TEMPOS
    printf("\n=== Tempos de Execucao ===\n");
    printf("Tempo total:         %10jd.%09ld s\n", total_time.tv_sec, total_time.tv_nsec);
    printf("Tempo paralelo:      %10jd.%09ld s\n", parallel_time.tv_sec, parallel_time.tv_nsec);
    printf("Tempo nao paralelo:  %10jd.%09ld s\n", non_parallel_time.tv_sec, non_parallel_time.tv_nsec);
    
    for (int t = 0; t < num_threads; t++) {
        struct timespec thread_time = diff_timespec(&thread_data[t].end_time, &thread_data[t].start_time);
        printf("Thread %d:            %10jd.%09ld s\n", t, thread_time.tv_sec, thread_time.tv_nsec);
    }
    
    //GUARDAR ESTATISTICAS
    char stats_file[MAX_PATH];
    snprintf(stats_file, MAX_PATH, "timing_%d%s.txt", num_threads, sort_mode);
    
    FILE *fp = fopen(stats_file, "w");
    if (fp) {
        // Tempo tota
        fprintf(fp, "%jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);
        
        //Tempo de cada thread
        for (int t = 0; t < num_threads; t++) {
            struct timespec thread_time = diff_timespec(&thread_data[t].end_time, &thread_data[t].start_time);
            fprintf(fp, "%jd.%09ld\n", thread_time.tv_sec, thread_time.tv_nsec);
        }
        
        //Tempo nao paralelo
        fprintf(fp, "%jd.%09ld\n", non_parallel_time.tv_sec, non_parallel_time.tv_nsec);
        
        fclose(fp);
        printf("\nEstatisticas guardadas em: %s\n", stats_file);
    } else {
        fprintf(stderr, "Erro ao criar ficheiro de estatisticas\n");
    }
    
    //LIBERTAR MEMORIA
    for (int i = 0; i < num_images; i++) {
        free(image_files[i]);
    }
    free(image_files);
    free(threads);
    free(thread_data);
    
    printf("\n=== Processamento Concluido ===\n");
    return 0;
}
