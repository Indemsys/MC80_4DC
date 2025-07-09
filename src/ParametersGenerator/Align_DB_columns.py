#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
====================================================================
Автономный скрипт для выравнивания столбцов в JSON-подобных файлах.

Этот скрипт извлечен из access_to_json.py и предназначен для
независимого использования для выравнивания столбцов в файлах
формата ParamsDB.txt.

Использование:
    python align_columns.py [путь_к_файлу]

Если путь не указан, скрипт будет искать файл по умолчанию в той же
директории, где находится скрипт.

Функциональность:
• Анализирует JSON-подобные файлы с таблицами в формате columns/rows
• Выравнивает столбцы для лучшей читаемости
• Создает резервную копию исходного файла
• Обрабатывает строковые и числовые значения с правильным выравниванием
====================================================================
"""

import json
import pathlib
import re
import shutil
import sys
import traceback
import copy
from typing import List

# ────────── пользовательские константы ───────────────────────────
# Определяем путь к директории скрипта и файл по умолчанию
SCRIPT_DIR = pathlib.Path(__file__).parent
DEFAULT_DB_FILE = "ParamsDB.txt"  # имя файла базы данных по умолчанию
# ──────────────────────────────────────────────────────────────────

def parse_data_row_elements(row_line_str: str) -> List[str]:
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
        return []

    content_str = processed_line[1:-1].strip()  # Содержимое между скобками
    if not content_str:  # Пустой массив '[]'
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

        if char == '\\':  # Обрабатываем экранирующий символ '\'
            current_element += char
            escape_next_char = True  # Следующий символ будет частью текущего элемента как есть
            continue

        if char == '"':
            in_quotes = not in_quotes
            current_element += char
        elif char == ',' and not in_quotes:  # Разделитель элементов, только если не внутри кавычек
            elements.append(current_element.strip())
            current_element = ""
        else:
            current_element += char

    elements.append(current_element.strip())  # Добавляем последний (или единственный) элемент
    return elements


def get_visual_length_for_align(s: str) -> int:
    """Возвращает визуальную длину строки (простая версия, использующая len)."""
    # Для более сложных случаев (например, символы CJK) может потребоваться другая логика.
    return len(s)


def align_columns_in_file(input_file_path: pathlib.Path, output_file_path: pathlib.Path):
    """Выравнивает столбцы в JSON-подобном текстовом файле и очищает лишние пробелы."""
    try:
        with open(input_file_path, 'r', encoding='utf-8') as f:
            file_content = f.read()
    except FileNotFoundError:
        print(f"Ошибка (align_columns_in_file): Файл {input_file_path} не найден.")
        return
    except Exception as e:
        print(f"Ошибка чтения (align_columns_in_file): {input_file_path} - {e}")
        return

    # Сначала очищаем лишние пробелы в ParameterDescription
    # Удалено: cleaned_data_dict = clean_parameter_descriptions(data_dict)
    # Вместо этого используется только clean_parameter_descriptions_inplace

    lines = file_content.splitlines(keepends=True)

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
                column_names_raw = json.loads(columns_json_str)  # Парсим имена столбцов
                if not isinstance(column_names_raw, list):  # Убедимся, что это список
                    raise ValueError("Распарсенные заголовки столбцов не являются списком.")
            except Exception as e:
                output_lines.append(line)  # Оставляем строку как есть
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
                        elements = parse_data_row_elements(lines[current_data_row_index])  # Передаем всю строку с отступами
                        all_data_rows_elements.append(elements)
                    elif row_line_content == ']' or row_line_content == '],':  # Конец блока "rows"
                        break
                    # Если строка не начинается с '[', это, вероятно, конец данных или другая секция
                    elif not row_line_content.startswith('['):
                        break
                    else:  # Неожиданный формат строки, прекращаем сбор данных для этого блока
                        break
                    current_data_row_index += 1

            if column_names_raw and all_data_rows_elements:  # Если есть и заголовки, и данные
                num_columns = len(column_names_raw)
                # Инициализируем максимальные ширины визуальной длиной имен заголовков (в кавычках)
                max_widths = [get_visual_length_for_align(json.dumps(name)) for name in column_names_raw]

                # Обновляем максимальные ширины на основе данных
                for row_elements in all_data_rows_elements:
                    for col_idx in range(num_columns):
                        element_str_representation = "null"  # Значение по умолчанию для отсутствующих элементов
                        if col_idx < len(row_elements):
                            element_str_representation = row_elements[col_idx]

                        current_len = get_visual_length_for_align(element_str_representation)
                        if col_idx < len(max_widths):
                            max_widths[col_idx] = max(max_widths[col_idx], current_len)
                        else:  # Случай, если данных больше, чем заголовков (маловероятно при правильном JSON)
                            max_widths.append(current_len)

                # Формируем новую выровненную строку заголовков
                aligned_column_names_str = []
                for idx, name in enumerate(column_names_raw):
                    quoted_name = json.dumps(name)  # Имя в кавычках, например, "Category"
                    padding = " " * (max_widths[idx] - get_visual_length_for_align(quoted_name))
                    aligned_column_names_str.append(quoted_name + padding)

                header_indent_match = re.match(r'^(\s*)', lines[header_line_index])
                header_indent_str = header_indent_match.group(1) if header_indent_match else "    "
                output_lines.append(f'{header_indent_str}"columns": [{", ".join(aligned_column_names_str)}],\n')

                # Добавляем строку "rows": [
                if rows_block_start_index != -1:
                    output_lines.append(lines[rows_block_start_index])

                original_data_lines_start_idx = rows_block_start_index + 1  # Определение переменной перемещено сюда

                # Вычисляем необходимое количество пробелов для выравнивания данных под заголовками
                data_line_leading_spaces_str = " "  # По умолчанию хотя бы один пробел
                if original_data_lines_start_idx < len(lines) and all_data_rows_elements:  # Убедимся, что есть хотя бы одна строка данных
                    first_data_line_str = lines[original_data_lines_start_idx]
                    first_data_line_indent_match = re.match(r'^(\s*)', first_data_line_str)
                    row_block_data_lines_indent_str = first_data_line_indent_match.group(1) if first_data_line_indent_match else "      "

                    # Количество пробелов = (отступ_заголовка + длина_префикса_заголовка("columns": [)) - (отступ_строки_данных + длина_префикса_данных([))
                    # Длина префикса заголовка до первого элемента = len("\"columns\": [") = 11
                    # Длина префикса данных до первого элемента = len("[") = 1
                    num_spaces_after_data_bracket = (len(header_indent_str) + 11) - (len(row_block_data_lines_indent_str) + 1)
                    if num_spaces_after_data_bracket < 0:
                        num_spaces_after_data_bracket = 1  # Минимум 1 пробел, если расчет странный
                    data_line_leading_spaces_str = " " * num_spaces_after_data_bracket

                # Формируем новые выровненные строки данных
                for row_idx, row_elements in enumerate(all_data_rows_elements):
                    aligned_row_elements_str = []
                    current_original_data_line_index = original_data_lines_start_idx + row_idx

                    if current_original_data_line_index >= len(lines):
                        continue  # Пропустить, если индекс выходит за пределы (маловероятно)

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
                continue  # Переходим к следующей итерации основного цикла while
            else:
                # Не удалось полностью обработать блок (например, нет данных для заголовков)
                output_lines.append(line)  # Добавляем исходную строку заголовков
                if rows_block_start_index != -1:  # Если нашли "rows": [, добавляем и ее
                    output_lines.append(lines[rows_block_start_index])
                    i = rows_block_start_index + 1
                else:
                    i += 1
        else:  # Строка не является строкой заголовков "columns" или не соответствует ожидаемому формату
            output_lines.append(line)
            i += 1

    try:
        with open(output_file_path, 'w', encoding='utf-8') as f:
            f.writelines(output_lines)
    except Exception as e:
        print(f"Ошибка записи выровненного файла (align_columns_in_file): {output_file_path} - {e}")


def align_columns_main(file_path: pathlib.Path = None):
    """Основная функция для запуска выравнивания столбцов в файле."""
    # Определяем путь к файлу
    if file_path is None:
        input_file_to_align = SCRIPT_DIR / DEFAULT_DB_FILE
    else:
        input_file_to_align = file_path

    # Сначала очищаем лишние пробелы в ParameterDescription
    clean_parameter_descriptions_inplace(str(input_file_to_align))

    # Создаем имя для временного файла, куда будет записан выровненный результат
    # Это предотвращает потерю данных, если что-то пойдет не так во время записи.
    temp_aligned_output_file = input_file_to_align.parent / (input_file_to_align.name + "._temp_aligned_")

    if not input_file_to_align.exists():
        print(f"Ошибка: Файл для выравнивания {input_file_to_align} не найден!")
        return False

    print(f"INFO: Запуск выравнивания столбцов для файла: {input_file_to_align}")

    # Создаем резервную копию
    backup_file = input_file_to_align.parent / (input_file_to_align.name + ".backup")
    try:
        shutil.copy2(str(input_file_to_align), str(backup_file))
        print(f"INFO: Создана резервная копия: {backup_file}")
    except Exception as e:
        print(f"Предупреждение: Не удалось создать резервную копию: {e}")

    try:
        align_columns_in_file(input_file_to_align, temp_aligned_output_file)

        # Если временный файл успешно создан и не пуст, перемещаем его на место исходного файла
        if temp_aligned_output_file.exists() and temp_aligned_output_file.stat().st_size > 0:
            shutil.move(str(temp_aligned_output_file), str(input_file_to_align))
            print(f"INFO: Выравнивание столбцов завершено. Файл {input_file_to_align} обновлен.")
            return True
        elif temp_aligned_output_file.exists():  # Файл создан, но пустой
            print(f"Предупреждение: Временный выровненный файл {temp_aligned_output_file} пуст. Исходный файл не изменен.")
            try:  # Пытаемся удалить пустой временный файл
                temp_aligned_output_file.unlink()
            except OSError:
                pass  # Игнорируем ошибку удаления, если не получилось
            return False
        else:  # Временный файл не был создан
            print(f"Предупреждение: Временный выровненный файл {temp_aligned_output_file} не был создан. Исходный файл не изменен.")
            return False

    except Exception as e:
        print(f"КРИТИЧЕСКАЯ ОШИБКА во время выравнивания столбцов: {e}")
        traceback.print_exc()  # Печатаем полный traceback для диагностики
        return False
    finally:
        # Гарантированно пытаемся удалить временный файл, если он все еще существует
        # (например, если shutil.move не удалось или была другая ошибка)
        if temp_aligned_output_file.exists():
            try:
                temp_aligned_output_file.unlink()
            except OSError as ose:
                print(f"Предупреждение: не удалось удалить временный файл {temp_aligned_output_file}: {ose}")


def clean_parameter_descriptions_inplace(filepath):
  """
  Удаляет лишние пробелы в поле ParameterDescription в массиве rows,
  не затрагивая форматирование файла.
  """
  import re
  import shutil

  # Создать резервную копию
  shutil.copy2(filepath, filepath + ".bak")

  with open(filepath, 'r', encoding='utf-8') as f:
    lines = f.readlines()

  # Найти индекс столбца ParameterDescription
  header_line = None
  for i, line in enumerate(lines):
    if '"columns"' in line and 'ParameterDescription' in line:
      header_line = i
      break
  if header_line is None:
    print("ParameterDescription column not found!")
    return
  # Получить индекс столбца ParameterDescription
  columns_line = lines[header_line]
  columns = eval(columns_line.split(':',1)[1].strip().rstrip(',').replace('null','None'))
  param_descr_idx = columns.index('ParameterDescription')

  # Регулярка для поиска строки массива row
  row_re = re.compile(r'^(\s*)\[(.*)\](,?)\s*$')

  def clean_spaces(s):
    # Удалить лишние пробелы внутри строки, но сохранить одиночные между словами
    return ' '.join(s.strip().split())

  for i, line in enumerate(lines):
    m = row_re.match(line)
    if m:
      indent, row_content, comma = m.groups()
      # Разбить строку на элементы массива (учитываем кавычки и запятые)
      # Простейший парсер: ищем все строки в кавычках
      items = []
      buf = ''
      in_str = False
      for c in row_content:
        if c == '"':
          in_str = not in_str
          buf += c
        elif c == ',' and not in_str:
          items.append(buf.strip())
          buf = ''
        else:
          buf += c
      if buf:
        items.append(buf.strip())
      # Если нужный индекс есть и это строка
      if len(items) > param_descr_idx:
        val = items[param_descr_idx]
        if val.startswith('"') and val.endswith('"'):
          inner = val[1:-1]
          cleaned = clean_spaces(inner)
          if cleaned != inner:
            items[param_descr_idx] = f'"{cleaned}"'
            # Собрать строку обратно
            new_row = indent + '[' + ', '.join(items) + ']' + comma + '\n'
            lines[i] = new_row

  with open(filepath, 'w', encoding='utf-8') as f:
    f.writelines(lines)


def main():
    """Основная функция для запуска скрипта из командной строки."""
    print("=== Скрипт выравнивания столбцов и очистки пробелов ===")
    print("Использование:")
    print("  python align_columns.py [путь_к_файлу]")
    print()

    file_path = None

    # Обрабатываем аргументы командной строки
    for arg in sys.argv[1:]:
        if not arg.startswith("-"):
            file_path = pathlib.Path(arg)    # Определяем файл для обработки
    if file_path is None:
        file_path = SCRIPT_DIR / DEFAULT_DB_FILE
        print(f"INFO: Используется файл по умолчанию: {file_path}")

    if not file_path.exists():
        print(f"Ошибка: Файл {file_path} не существует!")
        sys.exit(1)

    # Сначала очистка описаний
    clean_parameter_descriptions_inplace(str(file_path))

    # Затем выравнивание столбцов
    success = align_columns_main(file_path)
    operation = "выравнивание столбцов и очистка пробелов"

    if success:
        print(f"{operation.capitalize()} выполнена успешно!")
    else:
        print(f"Ошибка при выполнении операции: {operation}")
        sys.exit(1)


if __name__ == "__main__":
    main()
