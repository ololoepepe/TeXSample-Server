<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="ru_RU">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/application.cpp" line="419"/>
        <source>Storage already initialized</source>
        <comment>message</comment>
        <translation type="unfinished">Хранилище уже инициализировано</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="422"/>
        <source>Initializing storage...</source>
        <comment>message</comment>
        <translation type="unfinished">Инициализация хранилища...</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="425"/>
        <source>Error: No application instance</source>
        <comment>error</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="431"/>
        <source>Error: Failed to load texsample.sty</source>
        <comment>error</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="437"/>
        <source>Failed to load texsample.tex</source>
        <comment>error</comment>
        <translation type="unfinished">Не удалось загрузить texsample.tex</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="442"/>
        <location filename="../src/application.cpp" line="450"/>
        <source>Error:</source>
        <comment>error</comment>
        <translation type="unfinished">Ошибка:</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="446"/>
        <location filename="../src/application.cpp" line="456"/>
        <source>Done!</source>
        <comment>message</comment>
        <translation type="unfinished">Готово!</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="499"/>
        <source>Unknown command. Enter &quot;help --commands&quot; to see the list of available commands</source>
        <translation type="unfinished">Неизвестная команда. Введите &quot;help --commands&quot; чтобы посмотреть список доступных команд</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="620"/>
        <source>days</source>
        <translation type="unfinished">дней</translation>
    </message>
</context>
<context>
    <name>BSettingsNode</name>
    <message>
        <location filename="../src/application.cpp" line="532"/>
        <source>E-mail server address used for e-mail delivery</source>
        <translation>Адрес сервера эл. почты, используемый для доставки эл. почты</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="534"/>
        <source>E-mail server port</source>
        <translation>Порт сервера эл. почты</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="536"/>
        <source>Name of local host passed to the e-mail server</source>
        <translation>Имя локального хоста, передаваемое на сервер эл. почты</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="538"/>
        <source>Determines wether the e-mail server requires SSL connection</source>
        <translation>Определяет, требуется ли серверу эл. почты SSL-соединение</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="541"/>
        <source>Identifier used to log into the e-mail server</source>
        <translation>Идентификатор, используемый для входа на сервер эл. почты</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="545"/>
        <source>Password used to log into the e-mail server</source>
        <translation>Пароль, используемый для входа на сервер эл. почты</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="550"/>
        <source>Logging mode. Possible values:
  0 or less - don&apos;t log
  1 - log to console only
  2 - log to file only
  3 and more - log to console and file
  The default is 2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="557"/>
        <source>Logging the &quot;keep alive&quot; operations. Possible values:
  0 or less - don&apos;t log
  1 - log locally
  2 and more - log loaclly and remotely
  The default is 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Logging mode. Possible values:
0 or less - don&apos;t log
1 - log to console only
2 - log to file only
3 and more - log to console and file
The default is 2</source>
        <translation type="obsolete">Режим ведения логов. Возможные значения:
0 или меньше - не вести логов
1 - вести лог только в консоли
2 - вести лог только в файле
3 - вести лог и в консоли, и в файле
Значение по умолчанию - 2</translation>
    </message>
    <message>
        <source>Logging the &quot;keep alive&quot; operations. Possible values:
0 or less - don&apos;t log
1 - log locally
2 and more - log loaclly and remotely
The default is 0</source>
        <translation type="obsolete">Ведение логов операции &quot;keep alive&quot; (проверка соединения). Возможные значения:
0 или меньше - не вести логов
1 - вести лог локально
2 и более - вести лог и локально, и удалённо
Значение по умолчанию - 0</translation>
    </message>
</context>
<context>
    <name>BTerminalIOHandler</name>
    <message>
        <location filename="../src/application.cpp" line="564"/>
        <source>This is TeXSample Server. Enter &quot;help --all&quot; to see full Help</source>
        <translation>Это TeXSample Server. Введите &quot;help --all&quot; чтобы увидеть полную Справку</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="569"/>
        <source>Show for how long the application has been running</source>
        <translation>Показать, как долго работает приложение</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="573"/>
        <source>Show connected user count or list them all</source>
        <translation>Показать количество подключенных пользователей или перечислить их всех</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="576"/>
        <source>Show information about the user. The user may be specified by id or by login. Options:
  --connected-at - time when the user connected
  --info - detailed user information
  --uptime - for how long the user has been connected</source>
        <translation>Показать информацию о пользователе. Пользователь может быть указан при помощи идентификатора или логина. Опции:
  --connected-at - момент времени, когда пользователь подключился
  --info - подробная информация о пользователе
  --uptime - как долго пользователь был подключен</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="583"/>
        <source>Disconnect the specified user. If login is specified, all connections of this user will be closed</source>
        <translation>Отключить указанного пользователя. Если указан логин, то все соединения данного пользователя будут закрыты</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="588"/>
        <source>Start the server. If address is specified, the server will only listen on that address, otherwise it will listen on available all addresses.</source>
        <translation>Запустить сервер. Если указан адрес, то сервер будет принимать соединения только по этому адресу, иначе он будет принимать соединения по всем доступным адресам.</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="593"/>
        <source>Stop the server. Users are NOT disconnected</source>
        <translation>Остановить сервер. Пользователи НЕ отключаются</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="596"/>
        <source>Set the latest version of an application along with the download URL
  client must be either cloudlab-client (clab), tex-creator (tcrt), or texsample-console (tcsl)
  os must be either linux (lin, l), macos (mac, m), or windows (win, w)
  arch must be either alpha, amd64, arm, arm64, blackfin, convex, epiphany, risc, x86, itanium, motorola, mips, powerpc, pyramid, rs6000, sparc, superh, systemz, tms320, tms470</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Set the latest version of an application along with the download URL</source>
        <translation type="obsolete">Задать последнюю версию приложения вместе со ссылкой для загрузки</translation>
    </message>
</context>
<context>
    <name>Connection</name>
    <message>
        <location filename="../src/connection.cpp" line="50"/>
        <source>Invalid storage instance</source>
        <translation>Недействительный экземпляр хранилища</translation>
    </message>
</context>
<context>
    <name>DataSource</name>
    <message>
        <location filename="../src/datasource.cpp" line="138"/>
        <source>Invalid DataSource instance</source>
        <comment>error</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/datasource.cpp" line="142"/>
        <source>Failed to load database schema</source>
        <comment>error</comment>
        <translation type="unfinished">Не удалось загрузить схему базы данных</translation>
    </message>
    <message>
        <location filename="../src/datasource.cpp" line="153"/>
        <source>Can not create tables in read-only mode</source>
        <comment>error</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/datasource.cpp" line="158"/>
        <source>Database initialization failed</source>
        <comment>error</comment>
        <translation type="unfinished">Инициализация базы данных не удалась</translation>
    </message>
</context>
<context>
    <name>Global</name>
    <message>
        <source>E-mail already initialized</source>
        <translation type="obsolete">Эл. почта уже инициализирована</translation>
    </message>
    <message>
        <source>Initializing e-mail...</source>
        <translation type="obsolete">Инициализация эл. почты...</translation>
    </message>
    <message>
        <source>Enter e-mail server address:</source>
        <translation type="obsolete">Введите адрес сервера элю почты:</translation>
    </message>
    <message>
        <source>Invalid address</source>
        <translation type="obsolete">Недействительный адрес</translation>
    </message>
    <message>
        <source>Enter e-mail server port (default 25):</source>
        <translation type="obsolete">Введите порт сервера эл. почты (по умолчанию 25):</translation>
    </message>
    <message>
        <source>Invalid port</source>
        <translation type="obsolete">Недействительный порт</translation>
    </message>
    <message>
        <source>Enter local host name or empty string:</source>
        <translation type="obsolete">Введите имя локального хоста или пустую строку:</translation>
    </message>
    <message>
        <source>Enter SSL mode [true|false] (default false):</source>
        <translation type="obsolete">Введите режим SSL [true|false] (по умолчанию false):</translation>
    </message>
    <message>
        <source>Invalid value</source>
        <translation type="obsolete">Некорректное значение</translation>
    </message>
    <message>
        <source>Enter e-mail login:</source>
        <translation type="obsolete">Введите логин от эл. почты:</translation>
    </message>
    <message>
        <source>Invalid login</source>
        <translation type="obsolete">Недействительный логин</translation>
    </message>
    <message>
        <source>Enter e-mail password:</source>
        <translation type="obsolete">Введите пароль от эл. почты:</translation>
    </message>
    <message>
        <source>Invalid password</source>
        <translation type="obsolete">Недействительный пароль</translation>
    </message>
    <message>
        <source>Done!</source>
        <translation type="obsolete">Готово!</translation>
    </message>
    <message>
        <source>Printing password is unsecure! Do tou want to continue?</source>
        <translation type="obsolete">Вывод пароля небезопасен! Хотите продолжить?</translation>
    </message>
    <message>
        <source>Value for</source>
        <translation type="obsolete">Значение для</translation>
    </message>
    <message>
        <source>Enter logging mode:</source>
        <translation type="obsolete">Введите режим ведения логов:</translation>
    </message>
</context>
<context>
    <name>Storage</name>
    <message>
        <source>Storage already initialized</source>
        <translation type="obsolete">Хранилище уже инициализировано</translation>
    </message>
    <message>
        <source>Initializing storage...</source>
        <translation type="obsolete">Инициализация хранилища...</translation>
    </message>
    <message>
        <source>Failed to load texsample.sty</source>
        <translation type="obsolete">Не удалось загрузить texsample.sty</translation>
    </message>
    <message>
        <source>Failed to load texsample.tex</source>
        <translation type="obsolete">Не удалось загрузить texsample.tex</translation>
    </message>
    <message>
        <source>Failed to load database schema</source>
        <translation type="obsolete">Не удалось загрузить схему базы данных</translation>
    </message>
    <message>
        <source>Can&apos;t create tables in read-only mode</source>
        <translation type="obsolete">Нельзя создать таблицы в режиме только чтения</translation>
    </message>
    <message>
        <source>Can&apos;t create users in read-only mode</source>
        <translation type="obsolete">Нельзя создать пользователя в режиме только чтения</translation>
    </message>
    <message>
        <source>Database initialization failed</source>
        <translation type="obsolete">Инициализация базы данных не удалась</translation>
    </message>
    <message>
        <source>Invalid storage instance</source>
        <translation type="obsolete">Недействительный экземпляр хранилища</translation>
    </message>
    <message>
        <source>Failed to test invite codes</source>
        <translation type="obsolete">Не удалось проверить инвайт-коды</translation>
    </message>
    <message>
        <source>Failed to test recovery codes</source>
        <translation type="obsolete">Не удалось проверить коды восстановления</translation>
    </message>
    <message>
        <source>Enter superuser login:</source>
        <translation type="obsolete">Введите логин суперпользователя:</translation>
    </message>
    <message>
        <source>Enter superuser e-mail:</source>
        <translation type="obsolete">Введите электронную почту суперпользователя:</translation>
    </message>
    <message>
        <source>Enter superuser password:</source>
        <translation type="obsolete">Введите пароль сперпользователя:</translation>
    </message>
    <message>
        <source>Creating superuser account...</source>
        <translation type="obsolete">Создание аккаунта суперпользователя...</translation>
    </message>
    <message>
        <source>Operation aborted</source>
        <translation type="obsolete">Операция прервана</translation>
    </message>
    <message>
        <source>Done!</source>
        <translation type="obsolete">Готово!</translation>
    </message>
</context>
<context>
    <name>TerminalIOHandler</name>
    <message>
        <source>days</source>
        <translation type="obsolete">дней</translation>
    </message>
    <message>
        <source>There are no connected users</source>
        <translation type="obsolete">Отсутствуют подключенные пользователи</translation>
    </message>
    <message>
        <source>Connected user count:</source>
        <translation type="obsolete">Количество подключенных пользователей:</translation>
    </message>
    <message>
        <source>Listing connected users</source>
        <translation type="obsolete">Перечисляем подключенных пользователей</translation>
    </message>
    <message>
        <source>Uptime of</source>
        <translation type="obsolete">Аптайм</translation>
    </message>
    <message>
        <source>Connection time of</source>
        <translation type="obsolete">Время подключения</translation>
    </message>
    <message>
        <source>Uptime:</source>
        <translation type="obsolete">Аптайм:</translation>
    </message>
    <message>
        <source>Unknown command. Enter &quot;help --commands&quot; to see the list of available commands</source>
        <translation type="obsolete">Неизвестная команда. Введите &quot;help --commands&quot; чтобы посмотреть список доступных команд</translation>
    </message>
</context>
<context>
    <name>UserService</name>
    <message>
        <location filename="../src/service/userservice.cpp" line="175"/>
        <source>Can&apos;t create users in read-only mode</source>
        <comment>error</comment>
        <translation type="unfinished">Нельзя создать пользователя в режиме только чтения</translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="180"/>
        <source>Enter superuser login [default: &quot;root&quot;]:</source>
        <comment>prompt</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="185"/>
        <source>Enter superuser e-mail:</source>
        <comment>prompt</comment>
        <translation type="unfinished">Введите электронную почту суперпользователя:</translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="188"/>
        <source>Enter superuser password:</source>
        <comment>prompt</comment>
        <translation type="unfinished">Введите пароль сперпользователя:</translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="191"/>
        <source>Confirm password:</source>
        <comment>prompt</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="192"/>
        <source>Passwords does not match</source>
        <comment>error</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="193"/>
        <source>Enter superuser name [default: &quot;&quot;]:</source>
        <comment>prompt</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="196"/>
        <source>Enter superuser patronymic [default: &quot;&quot;]:</source>
        <comment>prompt</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="199"/>
        <source>Enter superuser surname [default: &quot;&quot;]:</source>
        <comment>prompt</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="214"/>
        <source>Creating superuser account...</source>
        <comment>message</comment>
        <translation type="unfinished">Создание аккаунта суперпользователя...</translation>
    </message>
    <message>
        <location filename="../src/service/userservice.cpp" line="216"/>
        <source>Failed to create superuser</source>
        <comment>error</comment>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../src/application.cpp" line="80"/>
        <source>This is</source>
        <translation>Это</translation>
    </message>
    <message>
        <location filename="../src/application.cpp" line="78"/>
        <source>read-only mode</source>
        <translation>режим только для чтения</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="41"/>
        <source>Enter &quot;help --commands&quot; to see the list of available commands</source>
        <translation>Введите &quot;help --commands&quot; чтобы посмотреть список доступных команд</translation>
    </message>
    <message>
        <source>Error:</source>
        <translation type="obsolete">Ошибка:</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="44"/>
        <source>Another instance of</source>
        <translation>Другой экземпляр</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="45"/>
        <source>is already running. Quitting...</source>
        <translation>уже запущен. Выходим...</translation>
    </message>
</context>
</TS>
