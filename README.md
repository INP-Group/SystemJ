
## Эксплуатация


Перед тем, как начинать журналировать канал, необходимо добавить его в БД.
Для этого:

- В файл resources/channels/to_add.txt
    - Добавьте список канало (каждый канал на новой строке)
- Запустите скрипт:
    ```bash
    python scripts/python/add_channels.py
    ```
    Этот скрипт добавит все нужные записи в нужные файлы


Чтобы обновить файлы со списком каналов запустите скрипт

```bash
python scripts/python/update_channels.py
```