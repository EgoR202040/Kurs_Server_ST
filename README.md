# Kurs_Server_ST
Использовать make, затем запуск с консоли ./server_egorow [параметры]
Параметры в данном случае необязательные и предусмотрен запуск с стандартными значениями
Параметры могут быть следующие:

--help - Вывод справки(не запускает сервер)
- p [--PORT] - Порт на котором сервер будет работать
- b [--basefile] - Путь к файлу с базой данных
- l [--logfile] - Путь к файлу для записи логов
--------------------------------------
Протокол работы с клиентом следующий:

1)Ожидается подключение от клиента

2)Открывается новый поток для клиента (максимальное кол-во одновременно работающих клиентов - 3)

3)Ожидается запрос от клиента в формате client_type + client_id + user_id (Запрос по сути хранит строку, которая является конкатенаций строк с параметрами)

4)Если найден такой user_id в базе, то сервер отправляет SALT (16 cлучайных hex цифр). Иначе разрывает соединение с клиентом

5)Сервер ожидает запроса от клиента с md5 хэшом конкатенации SALT + user_pass

6)Сервер отправляет подтверждение в виде запроса "OK"

7)Сервер в зависимости от ранее полученного client_type выбирает логику взаимодействия между регистрации нового пользователя или приём файла от клиента

7.1)Если выбрана регистрация, то сервер заранее проверяет роль для данного user_id и выносит решение, разрешено ли выполнять данную операцию

7.2)Если выбран приём файла, данной проверки нет
