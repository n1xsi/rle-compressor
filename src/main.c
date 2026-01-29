#include <stdio.h>    // Библиотека для ввода/вывода (например, printf, fprintf)
#include <stdlib.h>   // Библиотека для работы с памятью (например, malloc, free, exit)
#include <string.h>   // Библиотека для работы со строками (например, strcpy, strchr)
#include <dirent.h>   // Библиотека для работы с директориями (например, opendir, readdir)
#include <sys/stat.h> // Библиотека для работы с файловыми атрибутами (например, stat)
#include <unistd.h>   // Библиотека для системных вызовов (например, fseek)
#include <fcntl.h>    // Библиотека для управления файлами (например, open)

// Определение операционной системы
#ifdef _WIN32  // Windows
    #include <direct.h>
    #define MAKE_DIR(path) _mkdir(path)
#else         // Linux/MacOS
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MAKE_DIR(path) mkdir(path, 0755)
#endif

// Определение начального размера буфера
#define INITIAL_BUFFER_SIZE 1024

// Структура для хранения данных о файле
typedef struct
{
    char path[256];         // Путь к файлу
    size_t size;            // Размер файла
    unsigned char *content; // Указатель на содержимое файла
} FileEntry;

// Функция для удаления расширения из имени файла
void remove_extension(char *filename)
{
    char *dot = strrchr(filename, '.'); // Поиск последней точки в имени файла

    // Проверка наличия расширения
    if (dot && dot != filename)
    {
        *dot = '\0'; // Замена точки на конец строки (для удаления расширения)
    }
}

// Функция для получения расширения файла
const char *get_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.'); // Поиск последней точки в имени файла
    return dot ? dot + 1 : "";                // Возврат расширения или пустой строки
}

// Функция для сжатия данных методом RLE
size_t rle_compress(const unsigned char *input, size_t length, unsigned char **output)
{
    size_t buffer_size = INITIAL_BUFFER_SIZE; // Начальный размер буфера
    *output = malloc(buffer_size);            // Выделение памяти для сжатых данных

    // Проверка на успешное выделение памяти
    if (*output == NULL)
    {
        perror("Memory allocation failed"); // Сообщение об ошибке
        exit(1);                            // Завершение программы с кодом ошибки 1
    }

    size_t out_pos = 0; // Текущая позиция в буфере

    // Итерация по входным данным
    for (size_t i = 0; i < length;)
    {
        unsigned char byte = input[i]; // Текущий байт данных
        size_t count = 1;              // Счётчик повторяющихся байтов

        // Подсчёт последовательных одинаковых байтов (не более 255)
        while (i + count < length && input[i + count] == byte && count < 255)
        {
            count++;
        }

        // Увеличение буфера, если не хватает места для записи
        if (out_pos + 2 > buffer_size)
        {
            buffer_size *= 2;
            *output = realloc(*output, buffer_size);
            if (*output == NULL)
            {
                perror("Memory reallocation failed");
                exit(1);
            }
        }

        // Запись количества повторов и байта в выходной буфер
        (*output)[out_pos++] = count;
        (*output)[out_pos++] = byte;
        i += count; // Переход к следующему уникальному байту
    }
    return out_pos; // Возврат размера сжатых данных
}

// Функция для декодирования данных RLE
size_t rle_decompress(const unsigned char *input, size_t length, unsigned char **output)
{
    size_t buffer_size = INITIAL_BUFFER_SIZE; // Начальный размер буфера
    *output = malloc(buffer_size);            // Выделение памяти для декодированных данных

    // Проверка на успешное выделение памяти
    if (*output == NULL)
    {
        perror("Memory allocation failed");
        exit(1);
    }

    size_t out_pos = 0; // Текущая позиция в буфере

    // Итерация по входным данным
    for (size_t i = 0; i < length; i += 2)
    {
        unsigned char count = input[i];    // Количество повторений байта
        unsigned char byte = input[i + 1]; // Повторяющийся байт

        // Увеличение буфера, если не хватает места для записи
        if (out_pos + count > buffer_size)
        {
            while (out_pos + count > buffer_size)
            {
                buffer_size *= 2;
            }
            *output = realloc(*output, buffer_size);
            if (*output == NULL)
            {
                perror("Memory reallocation failed");
                exit(1);
            }
        }

        memset(*output + out_pos, byte, count); // Заполнение буфера байтом
        out_pos += count;                       // Увеличение позиции на количество записанных байтов
    }
    return out_pos; // Возврат размера декодированных данных
}

// Функция для чтения файла и сохранения его данных в структуре FileEntry
FileEntry *read_file(const char *path)
{
    FILE *file = fopen(path, "rb"); // Открытие файла для байтового чтения
    if (!file)
        return NULL; // Если файл не открылся - возврат NULL

    fseek(file, 0, SEEK_END);  // Переход к концу файла
    size_t size = ftell(file); // Определение размера файла
    fseek(file, 0, SEEK_SET);  // Возвращение в начало файла

    unsigned char *content = malloc(size); // Выделение памяти под содержимое файла
    fread(content, 1, size, file);         // Чтение данных из файла
    fclose(file);                          // Закрытие файла

    FileEntry *entry = malloc(sizeof(FileEntry));    // Выделение памяти под структуру FileEntry
    strncpy(entry->path, path, sizeof(entry->path)); // Сохранение пути файла
    entry->size = size;                              // Сохранение размера файла
    entry->content = content;                        // Сохранение содержимого файла
    return entry;                                    // Возвращение указателя (указатель на структуру)
}

// Функция для записи данных в файл
void write_file(const char *path, const unsigned char *content, size_t size)
{
    FILE *file = fopen(path, "wb"); // Открытие файла для записи
    fwrite(content, 1, size, file); // Запись данных в файл
    fclose(file);                   // Закрытие файла
}

// Рекурсивная функция для сжатия файлов в директории
void compress_directory(const char *path, FILE *output)
{
    struct dirent *entry;     // Указатель на запись директории
    DIR *dir = opendir(path); // Открытие директории
    if (!dir)
        return; // Если не удалось открыть директорию - выход

    // Чтение всех записей в директории
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // Пропуск текущей и родительской директории

        char full_path[512]; // Полный путь к файлу или директории
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        stat(full_path, &st); // Получение информации о файле или директории

        // Если это директория - применение рекурсивного сжатия
        if (S_ISDIR(st.st_mode))
        {
            compress_directory(full_path, output);
        }
        else
        {
            FileEntry *file_entry = read_file(full_path); // Чтение файла в структуру FileEntry
            if (!file_entry)
                continue; // Пропуск, если не удалось прочитать файл

            unsigned char *compressed;
            size_t compressed_size = rle_compress(file_entry->content, file_entry->size, &compressed);

            fwrite(file_entry->path, sizeof(file_entry->path), 1, output); // Запись пути файла
            fwrite(&compressed_size, sizeof(compressed_size), 1, output);  // Запись размера сжатых данных
            fwrite(compressed, compressed_size, 1, output);                // Запись сжатых данных

            free(compressed);          // Освобождение памяти для сжатых данных
            free(file_entry->content); // Освобождение памяти для содержимого файла
            free(file_entry);          // Освобождение памяти для структуры FileEntry
        }
    }
    closedir(dir); // Закрытие директории
}

// Функция для декомпрессии директории из сжатого файла
void decompress_directory(const char *output_path, FILE *input)
{
    // Чтение до конца файла
    while (!feof(input))
    {
        FileEntry file_entry;

        if (fread(file_entry.path, sizeof(file_entry.path), 1, input) != 1)
            break;
        size_t compressed_size;
        fread(&compressed_size, sizeof(compressed_size), 1, input); // Чтение размера сжатых данных

        unsigned char *compressed = malloc(compressed_size); // Выделение памяти для сжатых данных
        fread(compressed, compressed_size, 1, input);        // Чтение сжатых данных

        unsigned char *decompressed;
        size_t decompressed_size = rle_decompress(compressed, compressed_size, &decompressed);

        const char *relative_path = strchr(file_entry.path, '/');
        if (relative_path)
        {
            relative_path++; // Пропуск '/'
        }
        else
        {
            relative_path = file_entry.path; // Если корневой путь отсутствует
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", output_path, relative_path);

        char *dir_end = strrchr(full_path, '/');
        if (dir_end)
        {
            *dir_end = '\0';
            MAKE_DIR(full_path); // Создание директории
            *dir_end = '/';
        }

        write_file(full_path, decompressed, decompressed_size);

        free(compressed);
        free(decompressed);
    }
}

// Проверка, является ли сжатый файл директорией
int is_compressed_directory(FILE *input)
{
    char first_path[256];

    // Чтение первого пути в архиве и проверка, есть ли он
    if (fread(first_path, sizeof(first_path), 1, input) != 1)
        return 0;
    fseek(input, 0, SEEK_SET); // Возврат в начало файла

    // Если путь содержит '/' - файл представляет собой сжатую директорию
    return strchr(first_path, '/') != NULL;
}

// Функция для сжатия нескольких файлов
void compress_multiple_files(int file_count, char *file_paths[], FILE *output)
{
    for (int i = 0; i < file_count; i++)
    {
        // Чтение содержимого файла в структуру FileEntry
        FileEntry *file_entry = read_file(file_paths[i]);
        if (!file_entry)
            continue; // Пропуск, если файл не удалось прочитать

        unsigned char *compressed;

        // Сжатие данных с использованием RLE
        size_t compressed_size = rle_compress(file_entry->content, file_entry->size, &compressed);

        // Запись пути, размера и сжатого содержимого файла в выходной файл
        fwrite(file_entry->path, sizeof(file_entry->path), 1, output);
        fwrite(&compressed_size, sizeof(compressed_size), 1, output);
        fwrite(compressed, compressed_size, 1, output);

        // Очистка выделенной памяти
        free(compressed);
        free(file_entry->content);
        free(file_entry);
    }
}

// Функция для декомпрессии без создания директорий
void decompress_in_current_directory(FILE *input)
{
    // Чтение до конца сжатого файла
    while (!feof(input))
    {
        FileEntry file_entry;

        // Чтение пути и размера сжатых данных
        if (fread(file_entry.path, sizeof(file_entry.path), 1, input) != 1)
            break;
        size_t compressed_size;
        fread(&compressed_size, sizeof(compressed_size), 1, input);

        // Чтение сжатого содержимого
        unsigned char *compressed = malloc(compressed_size);
        fread(compressed, compressed_size, 1, input);

        // Декомпрессия данных
        unsigned char *decompressed;
        size_t decompressed_size = rle_decompress(compressed, compressed_size, &decompressed);

        // Извлечение имени файла из полного пути
        char *file_name = strrchr(file_entry.path, '/');
        file_name = file_name ? file_name + 1 : file_entry.path;

        // Запись декомпрессированного содержимого в текущую директорию
        write_file(file_name, decompressed, decompressed_size);

        // Освобождение памяти
        free(compressed);
        free(decompressed);
    }
}

// Главная функция программы
int main(int argc, char *argv[])
{
    // Проверка правильности количества аргументов
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <compress|decompress> <paths...>\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    struct stat st;

    // Проверка, является ли аргумент директорией для сжатия одного каталога
    int is_single_directory = (argc == 3 && stat(argv[2], &st) == 0 && S_ISDIR(st.st_mode));

    if (strcmp(command, "compress") == 0)
    {
        if (argc == 3 && is_single_directory)
        {
            char path[512];
            strncpy(path, argv[2], sizeof(path));
            remove_extension(path);
            char output_filename[512];
            snprintf(output_filename, sizeof(output_filename), "%s.rle", path);

            // Открытие файла для записи сжатой директории
            FILE *output = fopen(output_filename, "wb");
            if (!output)
            {
                perror("Could not open output file");
                return 1;
            }
            compress_directory(argv[2], output); // Сжатие содержимого директории
            fclose(output);
            printf("Compression complete: %s\n", output_filename);
        }
        else if (argc > 3)
        {
            // Если указано несколько файлов для сжатия
            FILE *output = fopen("compressed.rle", "wb");
            if (!output)
            {
                perror("Could not open output file");
                return 1;
            }
            compress_multiple_files(argc - 2, &argv[2], output); // Сжатие списка файлов
            fclose(output);
            printf("Compression complete: compressed.rle\n");
        }
        else
        {
            char path[512];
            strncpy(path, argv[2], sizeof(path));

            // Сжатие одного файла
            FileEntry *file_entry = read_file(argv[2]);
            if (!file_entry)
                return 1;

            unsigned char *compressed;
            size_t compressed_size = rle_compress(file_entry->content, file_entry->size, &compressed);

            remove_extension(path);
            char output_filename[512];
            snprintf(output_filename, sizeof(output_filename), "%s.rle", path);

            FILE *output = fopen(output_filename, "wb");
            if (!output)
            {
                perror("Could not open output file");
                return 1;
            }

            // Запись пути, размера и сжатого содержимого
            fwrite(file_entry->path, sizeof(file_entry->path), 1, output);
            fwrite(&compressed_size, sizeof(compressed_size), 1, output);
            fwrite(compressed, compressed_size, 1, output);

            // Освобождение памяти
            free(compressed);
            free(file_entry->content);
            free(file_entry);

            fclose(output);
            printf("Compression complete: %s\n", output_filename);
        }
    }
    else if (strcmp(command, "decompress") == 0)
    {
        // Декомпрессия файла
        FILE *input = fopen(argv[2], "rb");
        if (!input)
        {
            perror("Could not open input file");
            return 1;
        }

        // Проверка, является ли файл сжатой директорией
        if (is_compressed_directory(input))
        {
            char output_dir[512];
            remove_extension(argv[2]);
            snprintf(output_dir, sizeof(output_dir), "%s", argv[2]);
            MAKE_DIR(output_dir);                    // Создание выходной директории
            decompress_directory(output_dir, input); // Декомпрессия в директорию
            printf("Decompression complete: %s\n", output_dir);
        }
        else
        {
            decompress_in_current_directory(input); // Декомпрессия в текущую директорию
            printf("Decompression complete in current directory.\n");
        }
        fclose(input);
    }
    else
    {
        // Обработка неизвестной команды
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
