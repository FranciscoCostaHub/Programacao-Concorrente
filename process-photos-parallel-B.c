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
 
 // ESTRUT PARA INFORMACAO DAS IMAGENS
 typedef struct {
     char filename[256];
     long size;
 } ImageInfo;
 
 // ESTRUT PARA TAREFAS DAS IMAGENS
 typedef struct {
     char input_dir[MAX_PATH];
     char output_dir[MAX_PATH];
     char filename[256];
     int terminate;
 } ImageTask;
 
 // ESTRUT PARA ESTATISTICAS GLOBAIS
 typedef struct {
     int total_images;
     double total_time;
     pthread_mutex_t mutex; 
 } Statistics;
 
 // Estrutura para passar dados a cada thread
 typedef struct {
     int pipe_fd;
     Statistics *stats;
     int thread_id;
 } ThreadData;
 
 
 // Comparacao por nome (ordem alfabetica)
 int compare_by_name(const void *a, const void *b) {
     ImageInfo *img_a = (ImageInfo *)a;
     ImageInfo *img_b = (ImageInfo *)b;
     return strcmp(img_a->filename, img_b->filename);
 }
 
 // Comparacao por tamanho (ordem crescente)
 int compare_by_size(const void *a, const void *b) {
     ImageInfo *img_a = (ImageInfo *)a;
     ImageInfo *img_b = (ImageInfo *)b;
     return (img_a->size > img_b->size) - (img_a->size < img_b->size);
 }
 int file_exists(const char *filename) {
     return access(filename, F_OK) == 0;
 }
 void process_image(const char *input_path, const char *output_dir, const char *filename) {
     char output_path[MAX_PATH];
     gdImagePtr original, transformed;
     
     //LER IMG 
     original = read_jpeg_file((char *)input_path);
     if (!original) {
         fprintf(stderr, "\tErro ao ler %s\n", input_path);
         return;
     }
     
     //CONTRAST 
     snprintf(output_path, MAX_PATH, "%s/contrast_%s", output_dir, filename);
     transformed = contrast_image(original);
     if (transformed) {
         write_jpeg_file(transformed, output_path);
         gdImageDestroy(transformed);
     }
     
     //BLUR
     snprintf(output_path, MAX_PATH, "%s/blur_%s", output_dir, filename);
     transformed = blur_image(original);
     if (transformed) {
         write_jpeg_file(transformed, output_path);
         gdImageDestroy(transformed);
     }
     
     //SEPIA
     snprintf(output_path, MAX_PATH, "%s/sepia_%s", output_dir, filename);
     transformed = sepia_image(original);
     if (transformed) {
         write_jpeg_file(transformed, output_path);
         gdImageDestroy(transformed);
     }
     
     //THUMB
     snprintf(output_path, MAX_PATH, "%s/thumb_%s", output_dir, filename);
     transformed = thumb_image(original);
     if (transformed) {
         write_jpeg_file(transformed, output_path);
         gdImageDestroy(transformed);
     }
     
     //GRAY
     snprintf(output_path, MAX_PATH, "%s/gray_%s", output_dir, filename);
     transformed = gray_image(original);
     if (transformed) {
         write_jpeg_file(transformed, output_path);
         gdImageDestroy(transformed);
     }
     gdImageDestroy(original);
 }

 void print_statistics(Statistics *stats) {
     pthread_mutex_lock(&stats->mutex);
     
     if (stats->total_images > 0) {
         double avg_time = stats->total_time / stats->total_images;
         printf("Numero total de imagens processadas - %d\n", stats->total_images);
         printf("Tempo médio de processamento - %.2fs\n", avg_time);
     } else {
         printf("0 imagens - 0.0s tempo médio\n");
     }
     
     pthread_mutex_unlock(&stats->mutex);
 }

 int list_images(char *dir_path, ImageInfo *images) {
     DIR *dir = opendir(dir_path);
     if (!dir) {
         fprintf(stderr, "Erro ao abrir diretoria %s\n", dir_path);
         return 0;
     }
     
     int count = 0;
     struct dirent *entry;
     
     while ((entry = readdir(dir)) != NULL) {
         int len = strlen(entry->d_name);
         
         // VERIFICACAO SE TERMINA EM JPEG
         if (len > 5 && strcmp(entry->d_name + len - 5, ".jpeg") == 0) {
             strncpy(images[count].filename, entry->d_name, 255);
             images[count].filename[255] = '\0';
             
             //OBTEM O TAMANHO DO FICHEIOR
             char full_path[MAX_PATH];
             snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
             struct stat st;
             if (stat(full_path, &st) == 0) {
                 images[count].size = st.st_size;
             } else {
                 images[count].size = 0;
             }
             
             count++;
             if (count >= MAX_IMAGES) {
                 fprintf(stderr, "Aviso: Limite de %d imagens atingido\n", MAX_IMAGES);
                 break;
             }
         }
     }
     
     closedir(dir);
     return count;
 }

 void *thread_worker(void *arg) {
     ThreadData *data = (ThreadData *)arg;
     ImageTask task;
     
     while (1) {
         // AQUI ESPOERA POR TRABBALHO, BLOQUEIA ATE RECEBER DADOS
         ssize_t bytes_read = read(data->pipe_fd, &task, sizeof(ImageTask));
         
         if (bytes_read <= 0) {
             break;  /* Pipe fechado */
         }
         
         if (task.terminate) {
             break;
         }
         
         // PROCESSA A IMAGEM
         struct timespec start, end;
         clock_gettime(CLOCK_MONOTONIC, &start);
         
         char input_path[MAX_PATH];
         snprintf(input_path, MAX_PATH, "%s/%s", task.input_dir, task.filename);
         
         process_image(input_path, task.output_dir, task.filename);
         
         clock_gettime(CLOCK_MONOTONIC, &end);
         struct timespec processing_time = diff_timespec(&end, &start);
         double time_seconds = processing_time.tv_sec + 
                              processing_time.tv_nsec / 1000000000.0;
         
         //ATUALIXA AS ESTATISTICAS
         pthread_mutex_lock(&data->stats->mutex);
         
         data->stats->total_images++;
         data->stats->total_time += time_seconds;
         
         double avg_time = data->stats->total_time / data->stats->total_images;
         
         printf("thread %d processou %s em %.2fs\n", 
                data->thread_id, task.filename, time_seconds);
         printf("Numero total de imagens processadas - %d\n", 
                data->stats->total_images);
         printf("Tempo médio de processamento - %.2fs\n", avg_time);
         
         pthread_mutex_unlock(&data->stats->mutex);
     }
     
     return NULL;
 }

 int main(int argc, char *argv[]) {
     if (argc != 3) {
         fprintf(stderr, "Uso: %s <num_threads> <-name|-size>\n", argv[0]);
         fprintf(stderr, "Exemplo: %s 4 -size\n", argv[0]);
         exit(1);
     }
     
     int num_threads = atoi(argv[1]);
     char *sort_mode = argv[2];
     
     if (num_threads <= 0) {
         fprintf(stderr, "Erro: Numero de threads deve ser positivo\n");
         exit(1);
     }
     
     if (strcmp(sort_mode, "-name") != 0 && strcmp(sort_mode, "-size") != 0) {
         fprintf(stderr, "Erro: Modo de ordenacao deve ser -name ou -size\n");
         exit(1);
     }
     
     // CRIACAO DOS PIPES
     int pipes[num_threads][2];
     for (int i = 0; i < num_threads; i++) {
         if (pipe(pipes[i]) == -1) {
             perror("Erro ao criar pipe");
             exit(1);
         }
     }
     
     // INICIA AS ESTATISTICAS
     Statistics stats;
     stats.total_images = 0;
     stats.total_time = 0.0;
     pthread_mutex_init(&stats.mutex, NULL);
     
     // CRIACAO DAS THEREWDSA QUE VAO TRABAHAR
     pthread_t threads[num_threads];  // ESTE TEM DE TER _t!
     ThreadData thread_data[num_threads];
     
     for (int i = 0; i < num_threads; i++) {
         thread_data[i].pipe_fd = pipes[i][0];  /* fd de LEITURA */
         thread_data[i].stats = &stats;
         thread_data[i].thread_id = i;
         
         pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
     }
     
     printf("Foram criadas %d threads\n", num_threads);
     
     //CICLO DOS COMANDOS
     char linha[100], palavra_1[100], palavra_2[100];
     int should_quit = 0;
     int next_thread = 0;  /* Para distribuicao round-robin */
     
     while (!should_quit) {
         printf("Qual o comando: ");
         fflush(stdout);
         
         if (fgets(linha, 100, stdin) == NULL) {
             break;
         }
         
         int n_palavras = sscanf(linha, "%s %s", palavra_1, palavra_2);
         
         if (n_palavras >= 1) {
             //DIR
             if (strcmp(palavra_1, "DIR") == 0 && n_palavras == 2) {
                 char *input_dir = palavra_2;
                 
                 ImageInfo images[MAX_IMAGES];
                 int num_images = list_images(input_dir, images);
                 
                 if (num_images == 0) {
                     printf("Nenhuma imagem encontrada em %s\n", input_dir);
                     continue;
                 }
                 
                 //ORDENAR IMAGNENS
                 if (strcmp(sort_mode, "-name") == 0) {
                     qsort(images, num_images, sizeof(ImageInfo), compare_by_name);
                 } else {
                     qsort(images, num_images, sizeof(ImageInfo), compare_by_size);
                 }
                 
                 printf("A %d imagens na pasta %s serão processadas pelas %d threads\n",
                        num_images, input_dir, num_threads);
                 
                 // criar output
                char output_dir[MAX_PATH];
                snprintf(output_dir, MAX_PATH, "./Result-image-dir");
                create_directory(output_dir);
                 
                 for (int i = 0; i < num_images; i++) {
                     ImageTask task;
                     strncpy(task.input_dir, input_dir, MAX_PATH - 1);
                     strncpy(task.output_dir, output_dir, MAX_PATH - 1);
                     strncpy(task.filename, images[i].filename, 255);
                     task.filename[255] = '\0';
                     task.terminate = 0;
                     
                     write(pipes[next_thread][1], &task, sizeof(ImageTask));
                     
                     next_thread = (next_thread + 1) % num_threads;
                 }
             }
             //STAT
             else if (strcmp(palavra_1, "STAT") == 0) {
                 print_statistics(&stats);
             }
             //QUIT
             else if (strcmp(palavra_1, "QUIT") == 0) {
                 should_quit = 1;
                 
                 for (int i = 0; i < num_threads; i++) {
                     ImageTask task;
                     task.terminate = 1;
                     write(pipes[i][1], &task, sizeof(ImageTask));
                 }
             }
             else {
                 printf("comando inválido\n");
             }
         }
     }
     for (int i = 0; i < num_threads; i++) {
         pthread_join(threads[i], NULL);
         close(pipes[i][0]); 
         close(pipes[i][1]); 
     }
     
     print_statistics(&stats);
     
     pthread_mutex_destroy(&stats.mutex);
     
     return 0;
 }