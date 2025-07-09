#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""

====================================================================
Скрипт читает MS Access-базу **ParamsDB.mdb** (файл лежит рядом
со скриптом) и создаёт единый текстовый файл **ParamsDB.txt** –
валидный JSON-дамп с трансформацией данных и автоматическим
выравниванием столбцов.

Формат выходного файла:
• строка *columns* выводится одной строкой;
• каждая строка массива *rows* – тоже одной строкой;
• перед данными вставляется 9 пробелов (EXTRA_PAD) для «ровной» таблицы;
• столбцы автоматически выравниваются для лучшей читаемости.

────────────────────────────────────────────────────────────────────
ПРЕОБРАЗОВАНИЯ ДАННЫХ

DEVPARAMS
┌─ Перестановка и подстановка столбцов
│ • *sublevel* → **Category**
│   - берётся DevParamTree.CategoryName, где DevParamTree.ID == sublevel;
│   - переименован в Category и ставится 1-м столбцом.
│ • *SubNumber* становится 2-м столбцом.
│ • *SelectorID* → **Selector_name**
│   - строка из Selectors.SelectorName, где Selectors.Number == SelectorID;
│   - переименован в Selector_name и ставится 3-м столбцом.
│ • *ParameterType* (число) → строка DevVarTypes.VarTypeName
│   (по DevVarTypes.ID); переименован в Variable_type.
│ • *Parameter_variable_name* → **Variable_name**
└─ Генерация и очистка данных
  • *ParameterAlias* генерируется заново: 7 символов A-Z/0-9,
    никакого пробела внутри (алгоритм см. код).
  • Удаляются столбцы ID, ProfileID, PrevID, ParameterName.
  • После всех правок строки сортируются по (Category, SubNumber),
    а SubNumber перенумеровывается 1…N внутри каждой Category.

DEVPARAMTREE
• Удаляются ID, ProfileID, PrevID, PrevParentID, ShortDescription,
  поле `"Comment           "` (у Access-полей бывают хвостовые пробелы).
• Parent ID преобразуется в CategoryName для связи.
• CategoryName переименовывается в Category и перемещается на 1-е место.
• Данные сортируются по Parent, затем по Category.

SELECTORS
• Удаляются столбцы ID, ProfileID, Number.
• SelectorName переименовывается в Selector_name.
• SlectorDescription переименовывается в Selector_description.

SELECTORSLISTS
• SelectorID → строка Selectors.SelectorName (по Selectors.ID).
• Столбец переименовывается в Selector_name.
• Если после подстановки остаётся число или null → строка удаляется.
• Удаляется столбец ID, ProfileID.
• ValueStr преобразуется из строки в числа.
• Сортировка по Selector_name, затем по ValueStr.

DEVVARTYPES
• Удаляется столбец ID, ProfileID.
• VarTypeName очищается от пробелов и переименовывается в Variable_type.

DEVPROFILES
• Удаляется столбец ID, ProfileID.

Прочие таблицы
• Экспортируются без изменений, с удалением DROP-полей.

ВСТРОЕННОЕ ВЫРАВНИВАНИЕ СТОЛБЦОВ
После создания JSON-файла запускается автоматическое выравнивание:
• Анализируется ширина каждого столбца в данных.
• Для строковых значений пробелы добавляются внутрь кавычек.
• Для числовых значений пробелы добавляются снаружи.
• Создается визуально выровненная таблица для удобства чтения.
"""

import json
import os
import pathlib
import pyodbc
import random
import re
import shutil
import string
import traceback

# ────────── пользовательские константы ───────────────────────────
# Определяем путь к директории скрипта
SCRIPT_DIR = pathlib.Path(__file__).parent
DB_FILE   = SCRIPT_DIR / "ParamsDB.mdb"   # входная база
OUT_FILE  = SCRIPT_DIR / "ParamsDB.txt"   # горизонтальный JSON-дамп
EXTRA_PAD = 9                # отступ перед данными в строках rows

DROP_MAP: dict[str, list[str]] = {
    "DevParams": [
        "ID",
        "ProfileID",
        "PrevID",
        "ParameterName",
    ],
    "DevParamTree": [
        "ID",
        "ProfileID",
        "PrevID",
        "PrevParentID",
        "ShortDescription",
        "Comment           ",  # поле с хвостовыми пробелами
    ],
    "SelectorsLists": ["ID", "ProfileID"],
    "Selectors": ["ID", "ProfileID", "Number"],
    "DevVarTypes": ["ID", "ProfileID"],
    "DevProfiles": ["ID", "ProfileID"],
}
# ──────────────────────────────────────────────────────────────────


# ────────── вспомогательные функции ──────────────────────────────
def json_raw(val: object) -> str:
    """Вернуть «сырой» JSON-текст одного значения (без выравнивания)."""
    if val is None:
        return "null"
    if isinstance(val, bool):
        return "true" if val else "false"
    if isinstance(val, str):
        return '"' + val.replace('"', r'\"') + '"'
    if isinstance(val, float):
        # Округляем до 6 знаков после запятой
        rounded = round(val, 6)
        # Если после округления это целое число — выводим как int
        if rounded.is_integer():
            return str(int(rounded))
        else:
            # Форматируем с максимум 6 знаками после запятой, убираем лишние нули и точку
            s = f"{rounded:.6f}".rstrip('0').rstrip('.')
            if s == '':
                s = '0'
            return s
    return str(val)


def visual_len(val: object, header: bool = False) -> int:
    """Сколько печатных символов займёт значение (учёт кавычек/пробела)."""
    base = len(str(val)) if val is not None else len("null")
    if header:
        # Заголовки - простые строки без кавычек
        return base
    else:
        # Данные: строки с кавычками (+2) или числа/null с пробелом (+1)
        return base + (2 if isinstance(val, str) else 1)


def one_line(arr) -> str:
    """JSON-массив в одну строку."""
    return "[{}]".format(", ".join(json.dumps(x, ensure_ascii=False) for x in arr))


def make_alias(src: str, used: set[str]) -> str:
    """Сделать уникальную 7-символьную аббревиатуру из согласных букв всех слов."""
    import re

    # Согласные буквы (исключаем гласные A, E, I, O, U)
    consonants = set('BCDFGHJKLMNPQRSTVWXYZ')

    # Разделяем строку на слова по подчеркиванию или по смене регистра
    # Сначала заменяем подчеркивания на пробелы
    normalized = src.replace('_', ' ')
    # Затем добавляем пробелы перед заглавными буквами (для camelCase)
    normalized = re.sub(r'([a-z])([A-Z])', r'\1 \2', normalized)
    # Разделяем на слова
    words = normalized.split()

    # Собираем согласные из всех слов
    consonants_from_words = []
    for word in words:
        word_consonants = [ch.upper() for ch in word if ch.upper() in consonants]
        consonants_from_words.extend(word_consonants)

    # Формируем базовую аббревиатуру длиной 7 символов
    if len(consonants_from_words) >= 7:
        alias = ''.join(consonants_from_words[:7])
    else:
        # Если согласных меньше 7, дополняем повторением
        alias = ''.join(consonants_from_words)
        # Дополняем до 7 символов, повторяя согласные или добавляя X
        if consonants_from_words:
            # Повторяем согласные циклически
            while len(alias) < 7:
                alias += consonants_from_words[(len(alias) - len(consonants_from_words)) % len(consonants_from_words)]
        else:
            # Если согласных вообще нет, заполняем X
            alias = 'X' * 7

    # Проверяем уникальность
    if alias not in used:
        used.add(alias)
        return alias

    # Если аббревиатура не уникальна, изменяем последний символ
    attempts = 0
    max_attempts = len(string.ascii_uppercase + string.digits)

    for ch in string.ascii_uppercase + string.digits:
        alt = alias[:6] + ch
        if alt not in used:
            used.add(alt)
            return alt
        attempts += 1
        if attempts >= max_attempts:
            break

    # Если все варианты заняты, генерируем случайный
    import random
    for i in range(100):  # максимум 100 попыток
        random_alias = ''.join(random.choices(string.ascii_uppercase + string.digits, k=7))
        if random_alias not in used:
            used.add(random_alias)
            return random_alias

    raise RuntimeError(f"Не удалось создать уникальный alias для '{src}'")


def dump_horizontal_json(tables: dict, file_path: pathlib.Path) -> None:
    """Сохранить горизонтальный JSON-дамп."""
    lines: list[str] = ["{"]
    last_tbl = len(tables) - 1
    for i, (tbl_name, tbl) in enumerate(tables.items()):
        lines += [
            f'  "{tbl_name}": {{',
            f'    "columns": {one_line(tbl["columns"])},',
        ]

        # Проверяем, есть ли "rows" или другая структура данных
        if "rows" in tbl:
            lines.append('    "rows": [')
            for k, row in enumerate(tbl["rows"]):
                comma = "," if k < len(tbl["rows"]) - 1 else ""
                lines.append(
                    "      [ "
                    + " " * EXTRA_PAD
                    + ", ".join(row)
                    + f" ]{comma}"
                )
            lines.append('    ]')
        elif "data" in tbl:
            # Обработка специальной структуры для DevParamTree
            lines.append('    "data": {')
            data_items = list(tbl["data"].items())
            for j, (key, value) in enumerate(data_items):
                comma = "," if j < len(data_items) - 1 else ""
                lines.append(f'      "{key}": {json.dumps(value, ensure_ascii=False)}{comma}')
            lines.append('    }')
        else:
            # Fallback: пустой массив rows
            lines.append('    "rows": []')

        tail = "," if i < last_tbl else ""
        lines.append(f"  }}{tail}")
    lines += ["}"]
    file_path.write_text("\n".join(lines), encoding="utf-8")


# === ВСТРОЕННОЕ ВЫРАВНИВАНИЕ СТОЛБЦОВ (align_columns.py) ===

def parse_data_row_elements(row_line_str: str) -> list[str]:
    """
    Разбирает строку данных JSON-массива на элементы.
    Пример строки: '[ "element1", 123, "escaped \\"quote\\"", null ]'
    Возвращает список строковых представлений элементов.
    """
    processed_line = row_line_str.strip()
    # Удаляем возможную запятую в конце строки (если это последняя строка в массиве перед закрывающей ']')
    if processed_line.endswith(','):
        processed_line = processed_line[:-1]

    # Проверяем, что строка действительно является массивом
    if not (processed_line.startswith('[') and processed_line.endswith(']')):
        # print(f"DEBUG (parse_data_row_elements): Invalid row format: {row_line_str}")
        return []

    content_str = processed_line[1:-1].strip() # Содержимое между скобками
    if not content_str: # Пустой массив '[]'
        return []

    elements = []
    current_element = ""
    in_quotes = False
    escape_next_char = False

    for char in content_str:
        if escape_next_char:
            current_element += char
            escape_next_char = False
            continue

        if char == '\\\\': # Обрабатываем экранирующий символ '\'
            current_element += char
            escape_next_char = True # Следующий символ будет частью текущего элемента как есть
            continue

        if char == '"':
            in_quotes = not in_quotes
            current_element += char
        elif char == ',' and not in_quotes: # Разделитель элементов, только если не внутри кавычек
            elements.append(current_element.strip())
            current_element = ""
        else:
            current_element += char

    elements.append(current_element.strip()) # Добавляем последний (или единственный) элемент
    return elements

def get_visual_length_for_align(s: str) -> int:
    """Возвращает визуальную длину строки (простая версия, использующая len)."""
    # Для более сложных случаев (например, символы CJK) может потребоваться другая логика.
    return len(s)

def align_columns_in_file(input_file_path: pathlib.Path, output_file_path: pathlib.Path):
    """Выравнивает столбцы в JSON-подобном текстовом файле."""
    try:
        with open(input_file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Ошибка (align_columns_in_file): Файл {input_file_path} не найден.")
        return
    except Exception as e:
        print(f"Ошибка чтения (align_columns_in_file): {input_file_path} - {e}")
        return

    output_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]
        # Ищем строку с заголовками столбцов: "columns": ["col1", "col2"],
        if '"columns":' in line and line.strip().endswith('],'):
            header_line_index = i
            column_names_raw = []
            try:
                # Извлекаем JSON-массив имен столбцов
                columns_json_str = line.split('"columns":', 1)[1].rsplit(',', 1)[0].strip()
                if not (columns_json_str.startswith('[') and columns_json_str.endswith(']')):
                    raise ValueError("Формат заголовков столбцов не является списком.")
                column_names_raw = json.loads(columns_json_str) # Парсим имена столбцов
                if not isinstance(column_names_raw, list): # Убедимся, что это список
                    raise ValueError("Распарсенные заголовки столбцов не являются списком.")
            except Exception as e:
                # print(f"DEBUG (align_columns_in_file): Не удалось распарсить имена столбцов: {line.strip()}, ошибка: {e}")
                output_lines.append(line) # Оставляем строку как есть
                i += 1
                continue

            all_data_rows_elements = []
            rows_block_start_index = -1
            # Ищем начало блока "rows": [
            for j_search_rows in range(header_line_index + 1, len(lines)):
                if '"rows": [' in lines[j_search_rows].strip():
                    rows_block_start_index = j_search_rows
                    break

            if rows_block_start_index != -1:
                current_data_row_index = rows_block_start_index + 1
                # Собираем все строки данных из блока "rows"
                while current_data_row_index < len(lines):
                    row_line_content = lines[current_data_row_index].strip()
                    if row_line_content.startswith('[') and (row_line_content.endswith(']') or row_line_content.endswith('],')):
                        elements = parse_data_row_elements(lines[current_data_row_index]) # Передаем всю строку с отступами
                        all_data_rows_elements.append(elements)
                    elif row_line_content == ']' or row_line_content == '],': # Конец блока "rows"
                        break
                    # Если строка не начинается с '[', это, вероятно, конец данных или другая секция
                    elif not row_line_content.startswith('['):
                        break
                    else: # Неожиданный формат строки, прекращаем сбор данных для этого блока
                        break
                    current_data_row_index += 1

            if column_names_raw and all_data_rows_elements: # Если есть и заголовки, и данные
                num_columns = len(column_names_raw)
                # Инициализируем максимальные ширины визуальной длиной имен заголовков (в кавычках)
                max_widths = [get_visual_length_for_align(json.dumps(name)) for name in column_names_raw]

                # Обновляем максимальные ширины на основе данных
                for row_elements in all_data_rows_elements:
                    for col_idx in range(num_columns):
                        element_str_representation = "null" # Значение по умолчанию для отсутствующих элементов
                        if col_idx < len(row_elements):
                            element_str_representation = row_elements[col_idx]

                        current_len = get_visual_length_for_align(element_str_representation)
                        if col_idx < len(max_widths):
                            max_widths[col_idx] = max(max_widths[col_idx], current_len)
                        else: # Случай, если данных больше, чем заголовков (маловероятно при правильном JSON)
                            max_widths.append(current_len)

                # Формируем новую выровненную строку заголовков
                aligned_column_names_str = []
                for idx, name in enumerate(column_names_raw):
                    quoted_name = json.dumps(name) # Имя в кавычках, например, "Category"
                    padding = " " * (max_widths[idx] - get_visual_length_for_align(quoted_name))
                    aligned_column_names_str.append(quoted_name + padding)

                header_indent_match = re.match(r'^(\s*)', lines[header_line_index])
                header_indent_str = header_indent_match.group(1) if header_indent_match else "    "
                output_lines.append(f'{header_indent_str}"columns": [{", ".join(aligned_column_names_str)}],\n')

                # Добавляем строку "rows": [
                if rows_block_start_index != -1:
                    output_lines.append(lines[rows_block_start_index])

                original_data_lines_start_idx = rows_block_start_index + 1 # Определение переменной перемещено сюда

                # Вычисляем необходимое количество пробелов для выравнивания данных под заголовками
                data_line_leading_spaces_str = " " # По умолчанию хотя бы один пробел
                if original_data_lines_start_idx < len(lines) and all_data_rows_elements: # Убедимся, что есть хотя бы одна строка данных
                    first_data_line_str = lines[original_data_lines_start_idx]
                    first_data_line_indent_match = re.match(r'^(\s*)', first_data_line_str)
                    row_block_data_lines_indent_str = first_data_line_indent_match.group(1) if first_data_line_indent_match else "      "

                    # Количество пробелов = (отступ_заголовка + длина_префикса_заголовка("columns": [)) - (отступ_строки_данных + длина_префикса_данных([))
                    # Длина префикса заголовка до первого элемента = len("\"columns\": [") = 11
                    # Длина префикса данных до первого элемента = len("[") = 1
                    num_spaces_after_data_bracket = (len(header_indent_str) + 11) - (len(row_block_data_lines_indent_str) + 1)
                    if num_spaces_after_data_bracket < 0:
                        num_spaces_after_data_bracket = 1 # Минимум 1 пробел, если расчет странный
                    data_line_leading_spaces_str = " " * num_spaces_after_data_bracket

                # Формируем новые выровненные строки данных
                for row_idx, row_elements in enumerate(all_data_rows_elements):
                    aligned_row_elements_str = []
                    current_original_data_line_index = original_data_lines_start_idx + row_idx

                    if current_original_data_line_index >= len(lines):
                        # print(f"DEBUG: Index out of bounds for original_data_line: {current_original_data_line_index}")
                        continue # Пропустить, если индекс выходит за пределы (маловероятно)

                    original_data_line = lines[current_original_data_line_index]
                    row_line_indent_match = re.match(r'^(\s*)', original_data_line)
                    # Это отступ для самой строки данных, например "      " для "      [..."
                    current_row_indent_str = row_line_indent_match.group(1) if row_line_indent_match else "      "

                    for col_idx in range(num_columns):
                        element_str_representation = "null"
                        if col_idx < len(row_elements):
                            element_str_representation = row_elements[col_idx]

                        padding = " " * (max_widths[col_idx] - get_visual_length_for_align(element_str_representation))
                        aligned_row_elements_str.append(element_str_representation + padding)

                    original_row_trimmed = original_data_line.strip()
                    comma_suffix = "," if original_row_trimmed.endswith('],') else ""

                    # Формат строки данных: `      [<leading_spaces>"элемент1"  , null       , 123        ] `
                    output_lines.append(f'{current_row_indent_str}[{data_line_leading_spaces_str}{", ".join(aligned_row_elements_str)} ]{comma_suffix}\n')

                # Обновляем 'i', чтобы пропустить обработанные строки в исходном файле
                i = original_data_lines_start_idx + len(all_data_rows_elements)
                # Проверяем наличие закрывающей скобки ']' или '],' для блока "rows"
                if i < len(lines) and (lines[i].strip() == ']' or lines[i].strip() == '],'):
                    output_lines.append(lines[i])
                    i += 1
                continue # Переходим к следующей итерации основного цикла while
            else:
                # Не удалось полностью обработать блок (например, нет данных для заголовков)
                output_lines.append(line) # Добавляем исходную строку заголовков
                if rows_block_start_index != -1: # Если нашли "rows": [, добавляем и ее
                    output_lines.append(lines[rows_block_start_index])
                    i = rows_block_start_index + 1
                else:
                    i += 1
        else: # Строка не является строкой заголовков "columns" или не соответствует ожидаемому формату
            output_lines.append(line)
            i += 1

    try:
        with open(output_file_path, 'w', encoding='utf-8') as f:
            f.writelines(output_lines)
    except Exception as e:
        print(f"Ошибка записи выровненного файла (align_columns_in_file): {output_file_path} - {e}")

def align_columns_main():
    """Основная функция для запуска выравнивания столбцов в файле ParamsDB.txt."""
    # SCRIPT_DIR и OUT_FILE должны быть определены в глобальной области видимости основного скрипта
    # OUT_FILE это pathlib.Path объект
    input_file_to_align = OUT_FILE

    # Создаем имя для временного файла, куда будет записан выровненный результат
    # Это предотвращает потерю данных, если что-то пойдет не так во время записи.
    temp_aligned_output_file = SCRIPT_DIR / (input_file_to_align.name + "._temp_aligned_")

    if not input_file_to_align.exists():
        print(f"Предупреждение (align_columns_main): Файл для выравнивания {input_file_to_align} не найден! Выравнивание пропущено.")
        return

    print(f"INFO: Запуск выравнивания столбцов для файла: {input_file_to_align}")
    try:
        align_columns_in_file(input_file_to_align, temp_aligned_output_file)

        # Если временный файл успешно создан и не пуст, перемещаем его на место исходного файла
        if temp_aligned_output_file.exists() and temp_aligned_output_file.stat().st_size > 0:
            shutil.move(str(temp_aligned_output_file), str(input_file_to_align))
            print(f"INFO: Выравнивание столбцов завершено. Файл {input_file_to_align} обновлен.")
        elif temp_aligned_output_file.exists(): # Файл создан, но пустой
             print(f"Предупреждение (align_columns_main): Временный выровненный файл {temp_aligned_output_file} пуст. Исходный файл не изменен.")
             try: # Пытаемся удалить пустой временный файл
                 temp_aligned_output_file.unlink()
             except OSError: pass # Игнорируем ошибку удаления, если не получилось
        else: # Временный файл не был создан
            print(f"Предупреждение (align_columns_main): Временный выровненный файл {temp_aligned_output_file} не был создан. Исходный файл не изменен.")

    except Exception as e:
        print(f"КРИТИЧЕСКАЯ ОШИБКА во время выравнивания столбцов: {e}")
        traceback.print_exc() # Печатаем полный traceback для диагностики
    finally:
        # Гарантированно пытаемся удалить временный файл, если он все еще существует
        # (например, если shutil.move не удалось или была другая ошибка)
        if temp_aligned_output_file.exists():
            try:
                temp_aligned_output_file.unlink()
            except OSError as ose:
                print(f"Предупреждение (align_columns_main): не удалось удалить временный файл {temp_aligned_output_file}: {ose}")

# ────────── подключаемся к базе ──────────────────────────────────
try:
    conn = pyodbc.connect(
        rf'DRIVER={{Microsoft Access Driver (*.mdb, *.accdb)}};DBQ={os.path.abspath(DB_FILE)}'
    )
    cur = conn.cursor()
except FileNotFoundError:
    print(f"Ошибка: Файл базы данных '{DB_FILE}' не найден!")
    exit(1)
except Exception as e:
    print(f"Ошибка подключения к базе данных: {e}")
    exit(1)

# Справочники подстановок
cur.execute("SELECT ID, CategoryName FROM DevParamTree")
id_to_cat = {r.ID: r.CategoryName.strip() if isinstance(r.CategoryName, str) else r.CategoryName for r in cur.fetchall()}

cur.execute("SELECT Number, SelectorName FROM Selectors")
num_to_sel = {r.Number: r.SelectorName.strip() if isinstance(r.SelectorName, str) else r.SelectorName for r in cur.fetchall()}

cur.execute("SELECT ID, SelectorName FROM Selectors")
id_to_sel = {r.ID: r.SelectorName.strip() if isinstance(r.SelectorName, str) else r.SelectorName for r in cur.fetchall()}

cur.execute("SELECT ID, VarTypeName FROM DevVarTypes")
# Удаляем начальные/конечные пробелы из VarTypeName для консистентности
id_to_type = {
    r.ID: r.VarTypeName.strip() if isinstance(r.VarTypeName, str) else r.VarTypeName
    for r in cur.fetchall()
}

# список пользовательских таблиц
table_names = [
    t.table_name
    for t in cur.tables(tableType="TABLE")
    if not t.table_name.startswith("MSys")
]

export: dict[str, dict[str, list[str]]] = {}

# ────────── обработка каждой таблицы ─────────────────────────────
for tbl in table_names:
    cur.execute(f"SELECT * FROM [{tbl}]")
    columns = [c[0] for c in cur.description]
    rows = [list(r) for r in cur.fetchall()]    # ---------- DevVarTypes: очистка VarTypeName от пробелов и переименование --------------
    if tbl == "DevVarTypes" and "VarTypeName" in columns:
        vtn_idx = columns.index("VarTypeName")
        for r in rows:
            if vtn_idx < len(r) and isinstance(r[vtn_idx], str):
                r[vtn_idx] = r[vtn_idx].strip()
        # Переименование VarTypeName в Variable_type
        columns[vtn_idx] = "Variable_type"

    # ---------- DevParams --------------------------------------------------
    if tbl == "DevParams":
        # 1. Category
        idx = columns.index("sublevel")
        for r in rows:
            r[idx] = id_to_cat.get(r[idx], r[idx])
            r.insert(0, r.pop(idx))
        columns.pop(idx)
        columns.insert(0, "Category")        # 2. SelectorName
        if "SelectorID" in columns:
            si = columns.index("SelectorID")
            for r in rows:
                r[si] = num_to_sel.get(r[si], r[si])
            columns[si] = "Selector_name"

        # 3. SubNumber -> 2-й
        if "SubNumber" in columns:
            sn = columns.index("SubNumber")
            if sn != 1:
                for r in rows:
                    r.insert(1, r.pop(sn))
                columns.insert(1, columns.pop(sn))        # 4. ParameterType
        if "ParameterType" in columns:
            pt = columns.index("ParameterType")
            for r in rows:
                r[pt] = id_to_type.get(r[pt], r[pt])  # берём строку без изменений
            # Переименование ParameterType в Variable_type
            columns[pt] = "Variable_type"# 5. ParameterAlias
        if "ParameterAlias" in columns and "ParameterName" in columns:
            pa, pn = columns.index("ParameterAlias"), columns.index("ParameterName")
            used_alias: set[str] = set()
            for r in rows:
                r[pa] = make_alias(str(r[pn]), used_alias)        # 6. Selector_name → 3-й столбец, сортировка, нумерация
        if "Selector_name" in columns:
            sel_idx = columns.index("Selector_name")
            if sel_idx != 2:
                for r in rows:
                    r.insert(2, r.pop(sel_idx))
                columns.insert(2, columns.pop(sel_idx))

            cat_idx = columns.index("Category")
            sn_idx = columns.index("SubNumber")
            rows.sort(key=lambda r: (str(r[cat_idx]).lower(), r[sn_idx]))
            cur_cat, cnt = None, 0
            for r in rows:
                cnt = 1 if r[cat_idx] != cur_cat else cnt + 1
                cur_cat = r[cat_idx]
                r[sn_idx] = cnt

        # 7. Переименование Parameter_variable_name в Variable_name
        if "Parameter_variable_name" in columns:
            pvn_idx = columns.index("Parameter_variable_name")
            columns[pvn_idx] = "Variable_name"

    # ---------- DevParamTree: преобразование Parent из ID в CategoryName ----
    if tbl == "DevParamTree":
        # Создаем справочник ID -> CategoryName для быстрого поиска
        id_to_category = {}
        id_idx = columns.index("ID")
        cat_idx = columns.index("CategoryName")
        parent_idx = columns.index("Parent")

        for r in rows:
            id_to_category[r[id_idx]] = r[cat_idx].strip() if isinstance(r[cat_idx], str) else str(r[cat_idx])
            # Также очищаем CategoryName в самой строке
            r[cat_idx] = r[cat_idx].strip() if isinstance(r[cat_idx], str) else r[cat_idx]

        # Преобразуем Parent ID в CategoryName
        for r in rows:
            parent_id = r[parent_idx]
            current_id = r[id_idx]
            # Если Parent == ID, то это корневой элемент (Parent = null)
            if parent_id == current_id:
                r[parent_idx] = None
            else:
                # Заменяем ID родителя на его CategoryName
                r[parent_idx] = id_to_category.get(parent_id, parent_id)        # Переименовываем CategoryName в Category
        columns[cat_idx] = "Category"        # Переставляем Category на первое место
        if cat_idx != 0:
            # Перемещаем колонку Category на первое место
            for r in rows:
                r.insert(0, r.pop(cat_idx))
            columns.insert(0, columns.pop(cat_idx))

        # Сортируем по столбцу Parent (родительская категория), затем по столбцу Category
        parent_col_idx = columns.index("Parent") if "Parent" in columns else 1
        category_col_idx = columns.index("Category") if "Category" in columns else 0
        def sort_key(row):
            parent_val = row[parent_col_idx] if row[parent_col_idx] is not None else ""
            category_val = row[category_col_idx] if row[category_col_idx] is not None else ""
            return (parent_val, category_val)
        rows.sort(key=sort_key)# ---------- Selectors: переименование SelectorName в Selector_name -------
    if tbl == "Selectors":
        if "SelectorName" in columns:
            sn_idx = columns.index("SelectorName")
            columns[sn_idx] = "Selector_name"
        # Переименование SlectorDescription в Selector_description
        if "SlectorDescription" in columns:
            sd_idx = columns.index("SlectorDescription")
            columns[sd_idx] = "Selector_description"    # ---------- SelectorsLists: переименование SelectorID в Selector_name ----
    if tbl == "SelectorsLists" and "SelectorID" in columns:
        si = columns.index("SelectorID")
        rows = [r for r in rows if r[si] in id_to_sel and r[si] is not None]
        for r in rows:
            r[si] = id_to_sel[r[si]]        # Переименовываем столбец SelectorID в Selector_name
        columns[si] = "Selector_name"
        # Сортируем по столбцу Selector_name (первый столбец), затем по ValueStr (второй столбец) как числовому
        valuestr_idx = columns.index("ValueStr") if "ValueStr" in columns else 1
        def sort_key(row):
            selector_name = row[si] if row[si] is not None else ""
            # ValueStr находится по индексу valuestr_idx
            value_str = row[valuestr_idx] if len(row) > valuestr_idx and row[valuestr_idx] is not None else "0"
            try:
                value_num = int(value_str)
            except (ValueError, TypeError):
                value_num = 0
            return (selector_name, value_num)
        rows.sort(key=sort_key)
        # Преобразуем значения в столбце ValueStr из строк в числа
        for row in rows:
            if len(row) > valuestr_idx and row[valuestr_idx] is not None:
                try:
                    row[valuestr_idx] = int(row[valuestr_idx])
                except (ValueError, TypeError):
                    row[valuestr_idx] = 0# ---------- удаление DROP-полей ---------------------------------------
    for col in DROP_MAP.get(tbl, []):
        if col in columns:
            idx = columns.index(col)
            columns.pop(idx)
            for r in rows:
                if idx < len(r):
                    r.pop(idx)

    # ---------- вычислить ширины колонок для данных ----------------------------------
    widths = [0] * len(columns)
    for r in rows:
        for i, v in enumerate(r):
            if i < len(widths):
                json_formatted = json_raw(v)
                # Для строк учитываем кавычки, для чисел - нет, но добавляем пробел впереди
                if json_formatted.startswith('"'):
                    widths[i] = max(widths[i], len(json_formatted))
                else:
                    widths[i] = max(widths[i], len(json_formatted) + 1)  # +1 для пробела впереди# ---------- сформировать «красивые» строки ----------------------------
    pretty_rows: list[list[str]] = []
    for r in rows:
        line: list[str] = []
        for i, v in enumerate(r):
            raw = json_raw(v)

            if raw.startswith('"'):
                # строка с кавычками - добавляем пробелы после до полной ширины
                pad = widths[i] - len(raw)
                cell = raw + " " * pad
            else:
                # число / null – пробел перед и пробелы после до полной ширины
                pad = widths[i] - len(raw) - 1
                cell = " " + raw + " " * pad
            line.append(cell)
        pretty_rows.append(line)    # ---------- заголовки столбцов без добавления пробелов --------
    pretty_columns = columns  # используем заголовки как есть, без добавления пробелов

    export[tbl] = {
        "columns": pretty_columns,  # возвращаем выравнивание для названий столбцов
        "rows": pretty_rows,
    }

# ────────── запись файла -----------------------------------------------
try:
    dump_horizontal_json(export, OUT_FILE)
    print(f"Успех – создан {OUT_FILE}")

    # ────────── выравнивание столбцов встроенной функцией ───────────────
    try:
        align_columns_main() # This function is now defined below
    except Exception as align_error:
        print(f"Предупреждение: Не удалось выровнять столбцы: {align_error}")
        traceback.print_exc() # Added for more details on alignment error

except Exception as e:
    print(f"Ошибка записи файла: {e}")
    traceback.print_exc() # Added for more details on general error
finally:
    if 'conn' in locals() and conn: # Ensure conn is defined and not None
        conn.close()
