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
```bash
rsk39.tech/redis-adapter:<тэг версии>-amd64 -h
```
Запуск команд внутри контейнера из текущей директории с предварительно подготовленными файлами конфигурации в папке `./conf`:
```bash
docker run -it --rm -v $(pwd)/conf:/app/redis-adapter/conf rsk39.tech/redis-adapter:<тэг версии>-amd64 bash
```

## Доступные pipe директивы
Pipe - описание соединения различных рабочих и трафика между ними. 
TODO!

## Python modules
Можно поместить модуль написанный на питоне (аналог .lua скриптов) в папке, указанную аргументом --modules-path (по умолчанию ./modules/).

В данном файле доступны для импорта библиотеки:
* toml >= 0.10.2
* mergedeep >= 1.3.4
* aioredis >= 2.0.1
* aiomysql >= 0.1.1
* typing_extensions >= 4.2.0
* pyyaml >= 6.0
* pydantic >= 1.10.4
* debugpy >= 1.6.6
* aioconsole >= 0.6.1
* aioping >= 0.3.1
А также доступен модуль bootstrap. Он явлется основным интерфейсом. Логика должна быть реализована в классе наследнике bootstrap.Worker.
Данный модуль является шаблоном логики, обрабатывающей Json. В модуле должна быть выставлена переменная worker, указывающая на этот класс.

Пример:
```python
from bootstrap import BootParams, JsonState, Worker, JsonDict, Timer, Signal

class MyLogic(Worker):
    def __init__(self, params: BootParams) -> None:
        super().__init__(params)
    async def on_msg(self, msg: JsonDict):
        pass
    def on_run(self):
        self.create_task(self.spin)
    async def spin(self):
        while True:
            await do_work()

worker = MyLogic
```
В конфигурации redis-adapter можно указать следующее
```yaml
python:
  - worker:
      name: <module>
    module_path: <module>.py
    module_settings: 
        # passed to worker inside __init__(self, params)
        key: ...
    debug:
      enabled: false # enables debugpy remote debugging on specified port
      port: 5678
      wait: true # blocks startup until a client connects for remote dbug
```
