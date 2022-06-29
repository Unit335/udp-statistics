# UDP-statistics
## Описание

udp-statistics - набор утилит для Debian для сбора статистики по входящим UDP пакетам.

Сбор данных реализован в двух вариантах: первый (A) использует два потока, один из которых анализирует пакеты, а другую суммирует и отправляет статистику, второй выполняет анализ пакетов и суммирование статистики в одном потоке, а второй (B) только отправляет итоговые данные. В обеих случаях связь между потоками использует общие переменные. Выбор вариантов осуществляется при помощи аргументов

**udp-statshow** запрашивает у **udp-stat** собранные данные и выводит их в stdout. 

## Сборка

```sh
~$ git clone https://github.com/Unit335/udp-statistics
~$ cd udp-statistics
~$ make
```

Результатом сборки будут исполнительные файлы udp-statshow и udp-stat в папке репозитория.

## Запуск
По умолчанию программа запускается с вариантом работы A. 
В утилите используется следующий синтаксис для опционального управления филтрация пакетов - программа будет учитывать в статистике только пакеты с указанными параметрами:
```
udp-stat [ -A/B --interface INTERFACE --source SOURCE_IP --dest DESTINATION_IP --source_port SOURCE_PORT --dest_port DESTINATION_PORT ]
-A или -B: указывают вариант программы
interface: название сетевого интерфейса с которого просматривать пакеты
source: IP адрес источника в формате XXX.XXX.XXX.XXX
dest: IP адрес назначения в формате XXX.XXX.XXX.XXX 
source_port: порт источника
dest_port: destination port
```
Например:
```sh
~$ udp-statA -A --interface enp0s3 --dest 127.0.0.1 --dest-port 17 
Using version A
Starting
```
В случае отсутствия ошибок, программа не выводит никакой информации помимо индикации запуска "Starting...".

udp-statshow не использует никаких дополнительных аргументов при запуске.
```sh
~$ udp-statshow 
```
После запуска утилита будет с периодом в 2 с. выводить получаемые данные (количество и размер пакетов) на экран.

Протестировать сбор статистики по пакетам можно, например, следующим образом:

```sh
~$ sudo ./udp-stat --interface enp0s3 --source_port 5566 --dest_port 1022
~$ sudo ./udp-statshow
```
Записать произвольный набор описаний пакетов в .csv файл, например:
```
src_ip=10.0.0.1, dst_ip=10.0.0.2, src_port=5566, dst_port=1022, ether_type=ipv4
src_ip=10.0.0.1, dst_ip=10.0.0.2, src_port=5566, dst_port=1022, ether_type=ipv4
src_ip=10.0.0.1, dst_ip=10.0.0.2, src_port=5566, dst_port=1022, ether_type=ipv4
src_ip=20.20.20.21, dst_ip=20.20.20.22, src_port=5566, dst_port=1022, ether_type=ipv4
src_ip=20.20.20.21, dst_ip=20.20.20.22, src_port=5566, dst_port=1022, ether_type=ipv4
src_ip=10.0.0.1, dst_ip=10.0.0.2, src_port=12312, dst_port=514, ether_type=ipv4
```

С помощью https://github.com/cslev/pcap_generator сгенерировать набор пакетов: 
```sh
~$ git clone https://github.com/cslev/pcap_generator.git
~$ cd pcap_generator
~$ ./pcap_generator_from_csv.py -i input.csv -o udp-packets.pcap -v
```
И отправить пакеты направить пакеты с помощью tcpreplay на произвольный интерфейс
```
~$ sudo tcpreplay -i enp0s3 udp-packets.64bytes.pcap
``` 
Утилита udp-statshow отобразит пакеты, соответствующие указанным фильтрам (в данном случае 2 пакета на 120 байт).

## Профилирование
Для профилирования использовалась утилита perf. По итогам второй вариант **udp-statB** оказался существенно более эффективным. Время выполнения у потока анализа пакетов у двух вариантов было примерно одинаковых (в пределах доль миллисекунд при анализе на промежутке в 20с), при этом суммарное время выполнения задач второй утилиты было существенно меньше (1-1.5 msec к 16-18к msec у первой), т.к. в данном случае нет необходимости осуществлять отдельно передачу данных между потоками. 

Данные по вариантам A и B при передаче 6 UDP пакетов:
```sh
~$ sudo perf stat -p 3247 sleep 10 

 Performance counter stats for process id '3247':
         10 005,72 msec task-clock                #    0,999 CPUs utilized          
               128      context-switches          #    0,013 K/sec                  
                 0      cpu-migrations            #    0,000 K/sec                  
```
Вариант B
```sh
~$ sudo perf stat -p 3546 sleep 10 
              0,17 msec task-clock                #    0,000 CPUs utilized          
                 6      context-switches          #    0,036 M/sec                  
                 0      cpu-migrations            #    0,000 K/sec                  
```

## Авторство и лицензия
Дубовский Илья; idubovskii@outlook.com

Проект выпущен под лицензией MIT.
