ФИО: Ликанэ Роман Максимович
Группа: магистратура, 2 год обучения

Задание: 1 / Универсальный архиватор файлов
    база: [+]
    PPM1: [-]
    PPM2: [-]
    адаптивное сжатие: [+]
    индивидуальные таблицы: [+]

Система: Microsoft Visual Studio 2013, или GCC 4.8.4 + cmake
ОС: Windows 10, Ubuntu 14.04
Аппаратура: Intel Core i7, 8192 Mb, NVidia GeForce GT 520

Комментарии:
В программе есть два режима сжатия - adaptive, без сохранения таблицы частот, с ее накоплением по мере сжатия и распаковки, и custom - с предвычислением таблицы частот для файла и сохранением ее в архив.
Поведение по умолчанию: для файлов меньше 16Kb - adaptive, больше - custom
Максимально допустимый размер файла - 4Gb.

Поведение можно изменить, явно указав последним параметром в строке "--force-adaptive" для включения адаптивного сжатия и "--force-custom" для включения генерации кастомной таблицы

Формат файла архива:
/* File Format
 * 0: R     //magic
 * 1: L     //magic
 * 2: Z     //magic
 * 3: A/P   //arithm / ppm (ppm not implemented)
 * 4: C/A //model: custom/adaptive
 * [CTable ...] //Custom Freq Table (32 bit big-endian * 256) (custom only)
 * S3       //total inflated size big-endian
 * S2
 * S1
 * S0
 * [Data ...]   //Compressed code
 */

Поскольку в задаче явно указано, что сжимаем файл, а не поток байтов, я сознательно отказался от кодирования EOF-символа.
Вместо этого я записываю в архив размер несжатого файла и провожу разархивацию до получения файла нужного размера.

Тестирование проводилось на двух файлах - один заполнен случайными значениями, второй - текстовый лог.
Файл со случайными значениями, очевидно, сжимаем очень слабо, использовался для проверки корректности распаковки
Файл размером в 500 мегабайт сжимается и разжимается за ~4 минуты

Для тестирования был написан простой баш-скрипт test.sh:

test.sh
===cut here===
#!/bin/sh

./rlz c $1 $1.cmp $2
./rlz d $1.cmp $1.out

ls -la $1
ls -la $1.cmp

diff $1 $1.out
===cut here===


==========================================================
rl@rl:~/rlz-build$ dd if=/dev/urandom of=./test_rand bs=1k count=102400
102400+0 records in
102400+0 records out
104857600 bytes (105 MB) copied, 7.42415 s, 14.1 MB/s
==========================================================
rl@rl:~/rlz-build$ time ./test.sh ./test_rand
File ./test_rand loaded, size 104857600
Compression completed
File ./test_rand.cmp loaded, size 104940214
Decompression completed
-rw-rw-r-- 1 rl rl 104857600 Nov 12 11:01 ./test_rand
-rw-rw-r-- 1 rl rl 104940214 Nov 12 11:47 ./test_rand.cmp

real    2m34.502s
user    2m33.835s
sys     0m0.300s

==========================================================
rl@rl:~/rlz-build$ time ./test.sh ./test_rand --force-adaptive
File ./test_rand loaded, size 104857600
Compression completed
File ./test_rand.cmp loaded, size 104939300
Decompression completed
-rw-rw-r-- 1 rl rl 104857600 Nov 12 11:01 ./test_rand
-rw-rw-r-- 1 rl rl 104939300 Nov 12 11:53 ./test_rand.cmp

real    2m34.578s
user    2m33.914s
sys     0m0.277s

==========================================================
rl@rl:~/rlz-build$ time ./test.sh ./vw
File ./vw loaded, size 597306462
Compression completed
File ./vw.cmp loaded, size 341001782
Decompression completed
-rw-r----- 1 rl rl 597306462 Nov 12 11:17 ./vw
-rw-rw-r-- 1 rl rl 341001782 Nov 12 11:57 ./vw.cmp

real    3m59.298s
user    3m52.573s
sys     0m1.286s

==========================================================
rl@rl:~/rlz-build$ time ./test.sh ./vw --force-adaptive
File ./vw loaded, size 597306462
Compression completed
File ./vw.cmp loaded, size 341000964
Decompression completed
-rw-r----- 1 rl rl 597306462 Nov 12 11:17 ./vw
-rw-rw-r-- 1 rl rl 341000964 Nov 12 12:02 ./vw.cmp

real    3m57.714s
user    3m51.170s
sys     0m1.291s
