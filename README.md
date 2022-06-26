# UDP-statistics
## Описание

udp-statistics - набор утилит для Debian для сбора статистики по входящим UDP пакетам.

Сбор данных реализован в двух вариантах: **udp-statA** использует два потока, один из которых анализирует пакеты, а другую суммирует и отправляет статистику, **udp-statB** выполняет анализ пакетов и суммирование статистики в одном потоке, а второй только отправляет итоговые данные. В обеих случаях связь между потоками использует общие переменные.

**udp-statshow** запрашивает у **udp-statA** или **udp-statB** собранные данные и выводит их в stdout. 



## Сборка
Создать локальную копию репозитория и перейти в неё:
```sh
~$ git clone https://github.com/Unit335/udp-statistics
~$ cd udp-statistics
```
Скомпилировать выбранный вариант программы сбора статистики (udp-statA или udp-statB) с поддержкой потоков pthread и подключение библиотеки rt, например, для GCC (флаги -pthread и -lrt):
```sh
~$ udp-statistics$ gcc -o udp-statA -pthread udp-statA/main.c -lrt
```
И скомпилировать утилиту для вывода статистики (statshow):
```sh
~$ statshow$  gcc -o udp-statshow statshow/main.c -lrt
```
Результатом сборки будут исполнительные файлы udp-statshow и udp-stat в папке репозитория.

## Запуск
В утилите используется следующий синтаксис для опционального управления филтрация пакетов - программа будет учитывать в статистике только пакеты с указанными параметрами:
```
udp-stat [ --source SOURCE_IP --dest DESTINATION_IP --source_port SOURCE_PORT --dest_port DESTINATION_PORT ]
source: IP адрес источника в формате XXX.XXX.XXX.XXX
dest: IP адрес назначения в формате XXX.XXX.XXX.XXX 
source_port: порт источника
dest_port: destination port
```
Например:
```sh
~$ udp-statA --dest 127.0.0.1 --dest-port 17
Starting
```
В случае отсутствия ошибка, программа не выводит никакой информации помимо индикации запуска "Starting...".

udp-statshow не использует никаких дополнительных флагов при запуске.
```sh
~$ udp-statshow 
```
После запуска утилита будет с периодов 1 с выводить получаемые данные (количество и размер пакетов) на экран.

Протестировать сбор статистики по пакетам можно используя, например:
```
~$ echo "Hello" > /dev/udp/127.0.0.1/17
``` 

## Профилирование
Для профилирования использовалась утилита perf. По итогам второй вариант **udp-statB** оказался существенно более эффективным. Время выполнения у потока анализа пакетов у двух вариантов было примерно одинаковых (в пределах доль миллисекунд при анализе на промежутке в 20с), при этом суммарное время выполнения задач второй утилиты было существенно меньше (1-1.5 msec к 16-18к msec у первой), т.к. в данном случае нет необходимости осуществлять отдельно передачу данных между потоками. 

## Авторство и лицензия
Дубовский Илья; idubovskii@outlook.com
Проект выпущен под лицензией MIT.
