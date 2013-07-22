<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="ru_RU">
<context>
    <name>BSettingsNode</name>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="72"/>
        <source>E-mail server address used for e-mail delivery</source>
        <translation>Адрес сервера эл. почты, используемый для доставки эл. почты</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="75"/>
        <source>E-mail server port</source>
        <translation>Порт сервера эл. почты</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="77"/>
        <source>Name of local host passed to the e-mail server</source>
        <translation>Имя локального хоста, передаваемое на сервер эл. почты</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="80"/>
        <source>Determines wether the e-mail server requires SSL connection</source>
        <translation>Определяет, требуется ли серверу эл. почты SSL-соединение</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="83"/>
        <source>Identifier used to log into the e-mail server</source>
        <translation>Идентификатор, используемый для входа на сервер эл. почты</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="88"/>
        <source>Password used to log into the e-mail server</source>
        <translation>Пароль, используемый для входа на сервер эл. почты</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="93"/>
        <source>Logging mode. Possible values:
0 or less - don&apos;t log
1 - log to console only
2 - log to file only
3 and more - log to console and file
The default is 2</source>
        <translation type="unfinished">Режим ведения логов. Возможные значения:
0 или меньше - не вести логов
1 - вести лог только в консоли
2 - вести лог только в файле
3 - вести лог и в консоли, и в файле
Значение по умолчанию - 2</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="100"/>
        <source>Logging the &quot;keep alive&quot; operations. Possible values:
0 or less - don&apos;t log
1 - log locally
2 and more - log loaclly and remotely
The default is 0</source>
        <translation>Ведение логов операции &quot;keep alive&quot; (проверка соединения). Возможные значения:
0 или меньше - не вести логов
1 - вести лог локально
2 и более - вести лог и локально, и удалённо
Значение по умолчанию - 0</translation>
    </message>
</context>
<context>
    <name>BTerminalIOHandler</name>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="107"/>
        <source>This is TeXSample Server.
Enter &quot;help --all&quot; to see full Help</source>
        <translation>Это TeXSample Server.
Введите &quot;help --all&quot; чтобы увидеть полную Справку</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="112"/>
        <source>Show for how long the application has been running</source>
        <translation>Показать, как долго работает приложение</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="116"/>
        <source>Show connected user count or list them all</source>
        <translation>Показать количество подключенных пользователей или перечислить их всех</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="119"/>
        <source>Show information about the user.
The user may be specified by id or by login. Options:
  --connected-at - time when the user connected
  --info - detailed user information
  --uptime - for how long the user has been connected</source>
        <translation>Показать информацию о пользователе.
Пользователь может быть указан при помощи идентификатора или логина. Опции:
  --connected-at - момент времени, когда пользователь подключился
  --info - подробная информация о пользователе
  --uptime - как долго пользователь был подключен</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="126"/>
        <source>Disconnect the specified user.
If login is specified, all connections of this user will be closed</source>
        <translation>Отключить указанного пользователя.
Если указан логин, то все соединения данного пользователя будут закрыты</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="131"/>
        <source>Start the server.
If address is specified, the server will only listen on that address,
otherwise it will listen on available all addresses.</source>
        <translation>Запустить сервер.
Если указан адрес, то сервер будет принимать соединения только по этому адресу,
иначе он будет принимать соединения по всем доступным адресам.</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="136"/>
        <source>Stop the server. Users are NOT disconnected</source>
        <translation>Остановить сервер. Пользователи НЕ отключаются</translation>
    </message>
</context>
<context>
    <name>Connection</name>
    <message>
        <location filename="../src/connection.cpp" line="60"/>
        <source>Invalid storage instance</source>
        <translation>Недействительный экземпляр хранилища</translation>
    </message>
    <message>
        <location filename="../src/connection.cpp" line="599"/>
        <location filename="../src/connection.cpp" line="623"/>
        <source>Success!</source>
        <translation>Удачно!</translation>
    </message>
</context>
<context>
    <name>Global</name>
    <message>
        <location filename="../src/global.cpp" line="62"/>
        <source>E-mail already initialized</source>
        <translation>Эл. почта уже инициализирована</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="65"/>
        <source>Initializing e-mail...</source>
        <translation>Инициализация эл. почты...</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="69"/>
        <source>Enter e-mail server address:</source>
        <translation>Введите адрес сервера элю почты:</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="71"/>
        <source>Invalid address</source>
        <translation>Недействительный адрес</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="75"/>
        <source>Enter e-mail server port (default 25):</source>
        <translation>Введите порт сервера эл. почты (по умолчанию 25):</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="78"/>
        <source>Invalid port</source>
        <translation>Недействительный порт</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="81"/>
        <source>Enter local host name or empty string:</source>
        <translation>Введите имя локального хоста или пустую строку:</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="84"/>
        <source>Enter SSL mode [true|false] (default false):</source>
        <translation>Введите режим SSL [true|false] (по умолчанию false):</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="87"/>
        <source>Invalid value</source>
        <translation>Некорректное значение</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="91"/>
        <source>Enter e-mail login:</source>
        <translation>Введите логин от эл. почты:</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="93"/>
        <source>Invalid login</source>
        <translation>Недействительный логин</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="95"/>
        <location filename="../src/global.cpp" line="110"/>
        <source>Enter e-mail password:</source>
        <translation>Введите пароль от эл. почты:</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="97"/>
        <source>Invalid password</source>
        <translation>Недействительный пароль</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="104"/>
        <source>Done!</source>
        <translation>Готово!</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="116"/>
        <source>Printing password is unsecure! Do tou want to continue?</source>
        <translation>Вывод пароля небезопасен! Хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="119"/>
        <source>Value for</source>
        <translation>Значение для</translation>
    </message>
    <message>
        <location filename="../src/global.cpp" line="125"/>
        <source>Enter logging mode:</source>
        <translation>Введите режим ведения логов:</translation>
    </message>
</context>
<context>
    <name>Storage</name>
    <message>
        <location filename="../src/storage.cpp" line="59"/>
        <source>Storage already initialized</source>
        <translation>Хранилище уже инициализировано</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="62"/>
        <source>Initializing storage...</source>
        <translation>Инициализация хранилища...</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="66"/>
        <source>Failed to load texsample.sty</source>
        <translation>Не удалось загрузить texsample.sty</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="70"/>
        <source>Failed to load texsample.tex</source>
        <translation>Не удалось загрузить texsample.tex</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="78"/>
        <source>Failed to load database schema</source>
        <translation>Не удалось загрузить схему базы данных</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="91"/>
        <source>Can&apos;t create tables in read-only mode</source>
        <translation>Нельзя создать таблицы в режиме только чтения</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="96"/>
        <source>Can&apos;t create users in read-only mode</source>
        <translation>Нельзя создать пользователя в режиме только чтения</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="98"/>
        <source>Database initialization failed</source>
        <translation>Инициализация базы данных не удалась</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="101"/>
        <source>Invalid storage instance</source>
        <translation>Недействительный экземпляр хранилища</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="103"/>
        <source>Failed to test invite codes</source>
        <translation>Не удалось проверить инвайт-коды</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="105"/>
        <source>Failed to test recovery codes</source>
        <translation>Не удалось проверить коды восстановления</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="108"/>
        <source>Enter root e-mail:</source>
        <translation>Введите электронную почту для root:</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="110"/>
        <location filename="../src/storage.cpp" line="113"/>
        <source>Operation aborted</source>
        <translation>Операция прервана</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="127"/>
        <source>Done!</source>
        <translation>Готово!</translation>
    </message>
    <message>
        <location filename="../src/storage.cpp" line="111"/>
        <source>Enter root password:</source>
        <translation>Введите пароль для root:</translation>
    </message>
</context>
<context>
    <name>TerminalIOHandler</name>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="168"/>
        <source>days</source>
        <translation>дней</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="203"/>
        <source>There are no connected users</source>
        <translation>Отсутствуют подключенные пользователи</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="195"/>
        <source>Connected user count:</source>
        <translation>Количество подключенных пользователей:</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="201"/>
        <source>Listing connected users</source>
        <translation>Перечисляем подключенных пользователей</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="251"/>
        <source>Uptime of</source>
        <translation>Аптайм</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="256"/>
        <source>Connection time of</source>
        <translation>Время подключения</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="271"/>
        <source>Uptime:</source>
        <translation>Аптайм:</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="263"/>
        <source>Invalid parameters</source>
        <translation>Недействительные параметры</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="288"/>
        <location filename="../src/terminaliohandler.cpp" line="300"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="279"/>
        <source>The server is already running</source>
        <translation>Сервер уже запущен</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="150"/>
        <source>Unknown command. Enter &quot;help --commands&quot; to see the list of available commands</source>
        <translation>Неизвестная команда. Введите &quot;help --commands&quot; чтобы посмотреть список доступных команд</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="285"/>
        <source>Failed to start server</source>
        <translation>Не удалось запустить сервер</translation>
    </message>
    <message>
        <location filename="../src/terminaliohandler.cpp" line="296"/>
        <source>The server is not running</source>
        <translation>Сервер не запущен</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../src/main.cpp" line="52"/>
        <source>This is</source>
        <translation>Это</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="55"/>
        <source>read-only mode</source>
        <translation>режим только для чтения</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="71"/>
        <source>Enter &quot;help --commands&quot; to see the list of available commands</source>
        <translation>Введите &quot;help --commands&quot; чтобы посмотреть список доступных команд</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="68"/>
        <source>Error:</source>
        <translation>Ошибка:</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="80"/>
        <source>Another instance of</source>
        <translation>Другой экземпляр</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="81"/>
        <source>is already running. Quitting...</source>
        <translation>уже запущен. Выходим...</translation>
    </message>
</context>
</TS>
