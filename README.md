## Генерация примера конфигурации
```bash
./redis-adapter --dump-config-example > config.yaml.example
```

## Сборка Docker-контейнера
```bash
docker build --load --platform=linux/amd64 --build-arg TARGET_DEVICE=amd64 --build-arg JOBS=<потоков на сборку> -t rsk39.tech/redis-adapter:<тэг версии>-amd64 .
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