# README.md

## Сборка

> Описанные шаги и работа программы были протестированы только на Ubuntu 22.04.2 LTS.

> Предполагается что на машине установлены:
>  - Dev библиотеки для gstreamer
>  - Dev библиотеки для OpenCV 
>  - Компилятор g++
>  - СMake
>  - git

1. Клонируем репозиторий `git clone git@github.com:andreylitvintsev/NeurusTestTask1.git`
2. Переходим в директорию с репозиторием `cd ./NeurusTestTask1`
3. Создаем директорию где будет происходить сборка и переходим в нее`mkdir build && cd ./build`
4. Собираем проект `cmake ../ && cmake --build .`
5. Запускаем bash скрипт с заранее подставленными параметрами `bash start.sh`
