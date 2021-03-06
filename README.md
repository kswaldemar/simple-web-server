# Simple Web Server

Проект является заключительной работой [курса многопоточного программирования](https://stepic.org/course/%D0%9C%D0%BD%D0%BE%D0%B3%D0%BE%D0%BF%D0%BE%D1%82%D0%BE%D1%87%D0%BD%D0%BE%D0%B5-%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D0%B8%D1%80%D0%BE%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5-%D0%BD%D0%B0-%D0%A1%D0%A1++-149) на stepic.org
## Описание
Проект представляет собой упрощенный web-server

- Поддерживает несколько паралелльных соединений
- Обрабатывает только GET запросы
- Реализованные коды возврата: 200, 404

Сервер поддерживает параллельную обработку запросов, при помощи pthread.
Разбор протокола http является не приоритетной задачей, поэтому реализован не эффективно.

## Сборка
Для сборки проекта используется `cmake`
```
cmake .
make
```
## Запуск
При запуске необходимо задать следующие параметры:
```
-h host ip address
-p host port in which server running
-d root web server directory
-s stay on foreground
-w workers count
```
Например `final -h 127.0.0.1 -p 8080 -d /tmp/server`.
Где `final` - название исполняемого файла.

После запуска, если не указан ключ `-s`, сервер демонезируется и возвращает управление.

