# База данных параметров MC80 4DC - Схема и связи

## Обзор

Файл `ParamsDB.txt` содержит JSON-структуру базы данных параметров для системы MC80 4DC. База данных состоит из 6 взаимосвязанных таблиц, которые обеспечивают полное описание параметров системы, их типов, организации и интерфейса управления.

## Структура таблиц

### 1. DevParams - Основная таблица параметров
**Назначение:** Центральная таблица, содержащая все параметры системы
**Ключевые поля:** Category, Variable_type, Selector_name

**Столбцы:**
- `Category` - категория параметра (FK → DevParamTree.Category)
- `SubNumber` - номер в категории (1, 2, 3...)
- `Selector_name` - тип селектора UI (FK → Selectors.Selector_name)
- `ParameterDescription` - описание параметра
- `ParameterAlias` - короткий псевдоним (7 символов)
- `Variable_name` - имя переменной в коде
- `Variable_type` - тип данных (FK → DevVarTypes.Variable_type)
- `defval` - значение по умолчанию
- `minval` - минимальное значение
- `maxval` - максимальное значение
- `attr` - атрибуты (обычно "0")
- `pdefval` - предустановленное значение по умолчанию
- `format` - формат отображения (%d, %s, %0.1f)
- `func` - функция обработки (обычно "0")
- `varlen` - длина переменной (для строк)

### 2. DevParamTree - Иерархия категорий
**Назначение:** Организует параметры в древовидную структуру
**Связь:** Самосвязь через Parent → Category

**Столбцы:**
- `Category` - уникальное имя категории (PK)
- `Parent` - родительская категория (FK → DevParamTree.Category)
- `Description` - описание категории
- `Comment` - комментарий
- `Visible` - видимость в UI (True/False)
- `Nr` - порядковый номер для сортировки

### 3. DevVarTypes - Типы переменных
**Назначение:** Определяет соответствие между типами в БД и C-кодом
**Количество записей:** 8 типов

**Столбцы:**
- `Variable_type` - тип в БД (PK)
- `VarCType` - соответствующий C-тип
- `VarFMType` - тип для FreeMaster

### 4. Selectors - Типы селекторов интерфейса
**Назначение:** Определяет типы элементов управления в UI
**Количество записей:** 8 селекторов

**Столбцы:**
- `Selector_name` - имя селектора (PK)
- `Selector_description` - описание селектора

### 5. SelectorsLists - Значения для селекторов
**Назначение:** Содержит возможные значения для каждого типа селектора
**Количество записей:** 19 значений

**Столбцы:**
- `Selector_name` - имя селектора (FK → Selectors.Selector_name)
- `ValueStr` - строковое значение
- `Caption` - отображаемый текст
- `ImageIndx` - индекс иконки

### 6. DevProfiles - Профили устройств
**Назначение:** Определяет профили конфигурации
**Количество записей:** 1 профиль

**Столбцы:**
- `ProfileName` - имя профиля
- `Description` - описание профиля
- `StructureName` - имя структуры в коде

## Схема связей

### Связь 1: DevParams ↔ DevParamTree (Многие к одному)
```
DevParams.Category → DevParamTree.Category
```
**Описание:** Каждый параметр принадлежит определенной категории
**Примеры:**
- `MC80 4DC_AccessControl` → "Access control settings"
- `MC80 4DC_General` → "General settings"
- `MC80 4DC_Movement` → "Movement parameters"

### Связь 2: DevParams ↔ DevVarTypes (Многие к одному)
```
DevParams.Variable_type → DevVarTypes.Variable_type
```
**Описание:** Определяет тип данных C и FreeMaster для каждого параметра
**Примеры:**
- `tint8u` → `uint8_t` (FMSTR_TSA_UINT8)
- `tfloat` → `float` (FMSTR_TSA_FLOAT)
- `tstring` → `uint8_t` (FMSTR_TSA_UINT8)
- `tint32u` → `uint32_t` (FMSTR_TSA_UINT32)

### Связь 3: DevParams ↔ Selectors (Многие к одному)
```
DevParams.Selector_name → Selectors.Selector_name
```
**Описание:** Определяет тип элемента управления в пользовательском интерфейсе
**Примеры:**
- `string` → "Простая строка без выбора"
- `binary` → "Выбор между Yes и No"
- `inverter_type` → "Тип инвертера"

### Связь 4: Selectors ↔ SelectorsLists (Один ко многим)
```
Selectors.Selector_name → SelectorsLists.Selector_name
```
**Описание:** Для селекторов с выбором определяет возможные значения
**Примеры:**
- `binary` → ["0"="No", "1"="Yes"]
- `inverter_type` → ["0"="Lenze I550", "1"="Omron MX2", "2"="Goodrive GD20", "3"="Omron M1"]
- `emergency_lowering_sys` → ["0"="None", "1"="Mini-motor", "2"="External UPS", "3"="Backup controller"]

### Связь 5: DevParamTree ↔ DevParamTree (Самосвязь - иерархия)
```
DevParamTree.Parent → DevParamTree.Category
```
**Описание:** Создает древовидную структуру категорий

**Иерархия:**
```
MC80 4DC_0 (Root)
└── MC80 4DC_main (Parameters and settings)
    ├── MC80 4DC_General (General settings)
    ├── MC80 4DC_Movement (Movement parameters)
    ├── MC80 4DC_Service (Service settings)
    ├── MC80 4DC_Lighting (Lighting settings)
    ├── MC80 4DC_LinearEncoder (Linear Encoder settings)
    ├── MC80 4DC_Sounds (Sounds settings)
    ├── MC80 4DC_Inverter (Inverter settings)
    ├── MC80 4DC_AccessControl (Access control settings)
    ├── MC80 4DC_Brakes (Brake parameters)
    ├── MC80 4DC_Backup (Backup system settings)
    └── MC80 4DC_Display (Display settings)
        ├── MC80 4DC_SDisplay (Small Display settings)
        └── MC80 4DC_LDisplay (Large Display settings)
```

## Категории параметров по функциональности

### Управление системой
- **MC80 4DC_General** (10 параметров) - основные настройки системы
- **MC80 4DC_Service** (5 параметров) - сервисные настройки
- **MC80 4DC_Movement** (12 параметров) - параметры движения


### Безопасность и доступ
- **MC80 4DC_AccessControl** (13 параметров) - контроль доступа с ключами

### Оборудование
- **MC80 4DC_Inverter** (13 параметров) - настройки инвертера
- **MC80 4DC_Brakes** (3 параметра) - параметры тормозов
- **MC80 4DC_LinearEncoder** (3 параметра) - линейный энкодер [скрыто]

### Пользовательский интерфейс
- **MC80 4DC_SDisplay** (4 параметра) - малый дисплей
- **MC80 4DC_LDisplay** (1 параметр) - большой дисплей
- **MC80 4DC_Lighting** (2 параметра) - освещение
- **MC80 4DC_Sounds** (4 параметра) - звуковые сигналы

### Дополнительные функции
- **MC80 4DC_Backup** (2 параметра) - резервная система

## Типы данных и их использование

### Числовые типы
- `tint8u` (uint8_t) - булевые значения, перечисления (0-255)
- `tint32u` (uint32_t) - большие целые числа, таймауты
- `tint32s` (int32_t) - позиции  (могут быть отрицательными)
- `tfloat` (float) - вещественные числа (напряжения, токи, времена)

### Строковые типы
- `tstring` (uint8_t array) - строки конфигурации, ключи доступа

## Селекторы пользовательского интерфейса

### Простые селекторы
- `string` - поле ввода текста/числа
- `binary` - переключатель Yes/No

### Селекторы с выбором
- `inverter_type` - тип инвертера (4 варианта)
- `inverter_control_mode` - режим управления инвертером (3 варианта)
- `safety_unit_connection` - подключение блока безопасности (2 варианта)

## Примеры использования

### Поиск параметра по алиасу
```sql
-- Найти параметр по алиасу "ENKEYRA"
SELECT * FROM DevParams WHERE ParameterAlias = 'ENKEYRA'
-- Результат: en_key_reader_on_call_btn, tint8u, категория MC80 4DC_AccessControl
```

### Получение всех параметров категории
```sql
-- Все параметры управления доступом
SELECT * FROM DevParams WHERE Category = 'MC80 4DC_AccessControl'
-- 13 параметров: включение, ключи, таймаут
```

### Построение дерева категорий
```sql
-- Получить иерархию категорий
SELECT Category, Parent, Description FROM DevParamTree ORDER BY Nr
```

### Получение возможных значений селектора
```sql
-- Возможные значения для типа инвертера
SELECT ValueStr, Caption FROM SelectorsLists
WHERE Selector_name = 'inverter_type'
-- Результат: 0=Lenze I550, 1=Omron MX2, 2=Goodrive GD20, 3=Omron M1
```

## Соглашения об именовании

### Алиасы параметров (8 символов)
- Первые 4-6 символов - сокращение функции
- Последние символы - уникальный суффикс
- Примеры: ENKEYRA, ACCCTRL

### Имена переменных
- snake_case стиль
- Описательные имена

### Категории
- Префикс "MC80 4DC_"
- CamelCase после префикса
- Логическая группировка функций

## Особенности реализации

### Строковые параметры с списками
Некоторые параметры типа `tstring` содержат списки номеров этажей:
```
"1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16"
```

### Ключи доступа
Формат: `<key_id> <landings_list>`
```
"01A7A7A7000000A9 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16"
```

### Скрытые категории
Категория `MC80 4DC_LinearEncoder` имеет `Visible: False` - не отображается в UI.

### Связь с кодом
- `Variable_name` соответствует имени переменной в C-коде
- `Variable_type` определяет C-тип через DevVarTypes
- `format` используется для printf-форматирования
- `VarFMType` используется в FreeMaster для отладки

Этот документ служит справочником для понимания структуры базы данных параметров и может использоваться ИИ для автоматической обработки, генерации кода и анализа конфигурации системы MC80 4DC.
