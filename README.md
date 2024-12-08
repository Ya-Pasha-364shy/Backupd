# Backupd 2023

## Автор/Autor
Pavel Chernov (K1rch)

## Описание/Description
<b>ru:</b>
Backupd - это демон, производящий резервное копирование любых файлов, которые
были как-либо изменены в указанной директории или субдериктории указанной
директории. Резервное копирование производится сразу после внесения изменения
в файл (после сигнала CTRL+S, например).

Исключение для копирования - скрытые файлы, они игнорируются.

<b>en:</b>
Backupd is a daemon that backups any files that
have been modified in any way in the specified directory or subdirectory of the specified
directories. A backup is made immediately after the change is made.
to a file (after the CTRL+S signal, for example).

The exception for copying is hidden files, they are ignored.

## Сборка и установка/Assembly and installation

<b>ru:</b>
1. Выполните клонирование репозитория
```
git clone https://github.com/Ya-Pasha-364shy/Backupd.git
```

2. Запустите сборку `make`
```
make
```

ВНИМАНИЕ:
От Вас потребуются права root, чтобы интегрировать демона в свою систему !

3. Запустите демона через systemctl
```
systemctl start BackupPJ.service
```

4. Проверьте, что демон заработал
```
systemctl status BackupPJ.service
```

<i>
Теперь все файлы, которые были как-либо изменены в директории
указанной в конфигурационном файле (path_to_dir) или её субдиректории
будут скопированы в директорию, указанную в path_to_backup с
сохранением имени файла. 
</i>
Все лог-сообщения будут сохранены в директорию, указанную в параметре
path_to_log.

5. <b>Остановите работу демона</b>
```
systemctl stop BackupPJ.service
```
Таким образом демон остановится по сигналу SIGINT без утечек памяти.

<b>en:</b>
1. Clone the repo
```
git clone https://github.com/Ya-Pasha-364shy/Backupd.git
```

2. Run build `make`
```
make
```

<b>ATTENTION</b>:
You will need root privileges to integrate the daemon into your system!

3. Start the daemon via systemctl
```
systemctl start BackupPJ.service
```

4. Check that the daemon is running
```
systemctl status BackupPJ.service
```

<i>
Now all files that have been modified in any way in the directory
specified in the configuration file (path_to_dir) or its subdirectory
will be copied to the directory specified in path_to_backup with
saving the filename.
</i>
All log messages will be saved to the directory specified in the parameter
path_to_log.

5. <b>Stop the daemon</b>
```
systemctl stop BackupPJ.service
```
This way the daemon will stop on SIGINT signal without memory leaks.

## Дополнительно/Additionally
<b>ru:</b>
Если вы хотите сделать замечание, исправить найденный баг, сделать задачу из списка TODO, разбросанных по коду -
делайте merge-request. Названия ветки давайте таким образом:
bugfix-1.1 (если это первый найденный баг за проект), аналогично с фичами и модификациями (первое число - версия проекта, второе - номер исправленного бага, в данном случае).

Аналогично с фичами и модификациями:
feature-1.2
enhancement-1.3

<b>en:</b>
If you want to make a remark, fix a found bug, make a task from the TODO list scattered throughout the code -
make a merge request. Let's name the branch like this:
bugfix-1.1 (if this is the first bug found for the project), similarly with features and modifications (the first number is the project version, the second is the number of the fixed bug, in this case).

Similarly with features and modifications:
feature-1.2
enhancement-1.3

### Контактная информация/Contact info
<b>ru</b>:
Используйте телеграмм, для связи со мной. Ссылка на телеграм в <a href="https://github.com/K0001rch/K0001rch">профиле</a>

<b>en:</b>
Use telegram to contact me. Telegram <a href="https://github.com/K0001rch/K0001rch">link</a> in profile
