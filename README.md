## Генерация примера конфигурации
```bash
./redis-adapter --dump-config-example > config.yaml.example
```

## Сборка Docker-контейнера
```bash
./scripts/generate_version.sh
docker build --load --platform=linux/amd64 \
    --build-arg TARGET_DEVICE=amd64 \
    --build-arg JOBS=<потоков на сборку> \
    --build-arg APP_NAME=<имя .pro файла> \
    -t rsk39.tech/redis-adapter:<тэг версии>-amd64 .
```
Загрузка в Docker Hub:
```bash
docker push rsk39.tech/redis-adapter:<тэг версии>-amd64
```

## Использование
Запуск команд внутри контейнера из текущей директории с предварительно подготовленными файлами конфигурации в папке `./conf`:
```bash
docker run -it --rm -v $(pwd)/conf:/app/redis-adapter/conf rsk39.tech/redis-adapter:<тэг версии>-amd64 bash
```

## Доступные pipe директивы
Pipe - описание соединения различных рабочих и трафика между ними. 
TODO!