#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Params_generate.py
Автоматическая генерация {ProfileName}_Params.h и {ProfileName}_Params.c из ParamsDB.txt
"""
import os
import json
import re
import csv
from io import StringIO

PARAMS_DB_PATH = os.path.join(os.path.dirname(__file__), 'ParamsDB.txt')

# --- Утилиты ---
def crc16_ccitt(data):
    """
    Вычисление CRC16-CCITT для строки

    Args:
        data: строка для расчета CRC16

    Returns:
        int: значение CRC16
    """
    if isinstance(data, str):
        data = data.encode('utf-8')

    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc

# --- Утилиты ---
def load_params_db():
    with open(PARAMS_DB_PATH, encoding='utf-8') as f:
        text = f.read()
        # Удаляем многострочные комментарии /* ... */
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
        # Удаляем однострочные комментарии // ...
        text = re.sub(r'//.*', '', text)
        # Удаляем пустые строки
        text = '\n'.join([line for line in text.splitlines() if line.strip()])

        # Заменяем Python булевы значения на JSON
        text = re.sub(r'\bTrue\b', 'true', text)
        text = re.sub(r'\bFalse\b', 'false', text)

        # Заменяем null на правильные JSON значения
        text = re.sub(r'\bnull\b', 'null', text)

        # Удаляем висячие запятые (несколько проходов для вложенных структур)
        for _ in range(5):  # Максимум 5 проходов
            old_text = text
            text = re.sub(r',(\s*[}\]])', r'\1', text)
            if old_text == text:  # Если изменений нет, прекращаем
                break

        try:
            return json.loads(text)
        except json.JSONDecodeError as e:
            # Детальная диагностика ошибки
            lines = text.splitlines()
            line_num = getattr(e, 'lineno', 1) - 1
            col_num = getattr(e, 'colno', 1) - 1

            print(f"JSON Parse Error: {e}")
            print(f"Error at line {line_num + 1}, column {col_num + 1}")

            # Показываем контекст ошибки
            start_line = max(0, line_num - 3)
            end_line = min(len(lines), line_num + 4)

            print("\nContext around error:")
            for i in range(start_line, end_line):
                marker = " --> " if i == line_num else "     "
                print(f"{marker}{i+1:3}: {lines[i]}")
                if i == line_num and col_num < len(lines[i]):
                    print(f"     {' ' * (col_num + 4)}^")

            # Сохраняем очищенный файл для отладки
            debug_path = PARAMS_DB_PATH.replace('.txt', '_cleaned_debug.txt')
            with open(debug_path, 'w', encoding='utf-8') as debug_f:
                debug_f.write(text)
            print(f"\nCleaned text saved to: {debug_path}")

            raise SystemExit("ParamsDB.txt is not valid JSON after cleanup. Check the context above.")

def snake_case(name):
    return re.sub(r'([A-Z])', r'_\1', name).lower().lstrip('_')

def upper_snake(name):
    return snake_case(name).upper()

# --- Генерация .h ---
def generate_h(db):
    # Получаем имя профиля из DevProfiles
    profile_name = db['DevProfiles']['rows'][0][0] if db['DevProfiles']['rows'] else 'MC80'
    header_guard = f'{profile_name.upper()}_PARAMS_H'

    lines = []
    lines.append(f'#ifndef {header_guard}')
    lines.append(f'#define {header_guard}')
    lines.append('')
    lines.append('// This file is auto-generated. Do not edit manually.')
    lines.append('')

    # Category constants from DevParamTree
    lines.append('// Category constants')
    categories = db['DevParamTree']['rows']

    # Находим максимальную длину имени категории для выравнивания
    max_cat_len = max(len(row[0]) for row in categories) if categories else 0

    for i, row in enumerate(categories):
        category_name = row[0]
        lines.append(f'#define {category_name:<{max_cat_len}} {i}')
    lines.append('')

    # Типы
    vartypes = {row[0]: row[1] for row in db['DevVarTypes']['rows']}

    # Структура параметров (wvar) с выравниванием комментариев
    lines.append('typedef struct')
    lines.append('{')

    # Сначала собираем все строки для подсчёта максимальной длины
    struct_lines = []
    max_len = 0

    for row in db['DevParams']['rows']:
        ctype = vartypes.get(row[6], 'uint8_t')
        varname = row[5]
        comment = row[3]
        varlen = int(row[14]) if row[14] else 0  # varlen is at index 14

        # For tstring type, use array declaration
        if row[6] == 'tstring' and varlen > 0:
            var_decl = f'  {ctype} {varname}[{varlen}];'
        elif varlen > 0 and row[6] != 'tstring':
            var_decl = f'  {ctype} {varname}[{varlen}];'
        else:
            var_decl = f'  {ctype} {varname};'

        struct_lines.append((var_decl, comment))
        max_len = max(max_len, len(var_decl))

    # Добавляем строки с выравниванием комментариев
    for var_decl, comment in struct_lines:
        padding = max_len - len(var_decl) + 2
        lines.append(f'{var_decl}{" " * padding}// {comment}')

    # Получаем имя структуры из DevProfiles для формирования типа
    struct_name = db['DevProfiles']['rows'][0][2] if db['DevProfiles']['rows'] else 'wvar'
    wvar_type = f'{struct_name.upper()}_TYPE'
    lines.append(f'}} {wvar_type};')
    lines.append('')

    # Selector constants в формате SELECTOR_NAME_CAPTION
    lines.append('// Selector constants')
    selectors_lists = db['SelectorsLists']['rows']

    # --- Фильтрация используемых селекторов ---
    used_selectors = set(row[2] for row in db['DevParams']['rows'] if row[2])

    # Группируем по selector_name только используемые
    selector_groups = {}
    for row in selectors_lists:
        selector_name = row[0]
        if selector_name not in used_selectors:
            continue
        value = row[1]
        caption = row[2]
        if selector_name not in selector_groups:
            selector_groups[selector_name] = []
        selector_groups[selector_name].append((value, caption))

    # Генерируем константы для каждого селектора
    for selector_name, items in selector_groups.items():
        lines.append(f'// {selector_name}')

        # Находим максимальную длину имени макроса для выравнивания в этой группе
        safe_selector = upper_snake(selector_name)
        macro_names = []
        for value, caption in items:
            safe_caption = re.sub(r'[^a-zA-Z0-9]', '_', caption).upper()
            macro_names.append(f'{safe_selector}_{safe_caption}')

        max_macro_len = max(len(name) for name in macro_names) if macro_names else 0

        for i, (value, caption) in enumerate(items):
            safe_caption = re.sub(r'[^a-zA-Z0-9]', '_', caption).upper()
            macro_name = f'{safe_selector}_{safe_caption}'
            lines.append(f'#define {macro_name:<{max_macro_len}} {value}')
        lines.append('')    # extern
    struct_name = db['DevProfiles']['rows'][0][2] if db['DevProfiles']['rows'] else 'wvar'
    wvar_type = f'{struct_name.upper()}_TYPE'
    wvar_inst_name = f'{struct_name}_inst'
    lines.append(f'extern {wvar_type} {struct_name};')
    lines.append(f'extern const T_NV_parameters_instance {wvar_inst_name};')
    lines.append('')    # Function declarations for parameter hash lookup
    lines.append('// Function for fast parameter lookup by CRC16 hash')
    lines.append('uint16_t Find_param_by_hash(uint16_t hash);')
    lines.append('')
    lines.append('// Function to get parameter hash by index for CAN transmission')
    lines.append('uint16_t Get_param_hash_by_index(uint16_t index);')
    lines.append('')

    lines.append(f'#endif // {header_guard}')
    return '\n'.join(lines)

def generate_parmenu(db):
    """Генерирует массив parmenu из DevParamTree (parent, category, ...), исключая записи с parent == NULL."""
    lines = []
    rows = db['DevParamTree']['rows']
    # Фильтруем строки, исключая те где parent == NULL
    filtered_rows = [row for row in rows if row[1] is not None]

    lines.append(f'static const T_parmenu parmenu[{len(filtered_rows)}] =')
    lines.append('{')
    for row in filtered_rows:
        parent = row[1]
        category = row[0]
        descr = row[2] if row[2] else ''
        comment = row[3] if row[3] else ''
        visible = row[4] if row[4] is not None else True  # Используем поле Visible (индекс 4)
        visible_flag = 1 if visible else 0  # Преобразуем boolean в число 1 или 0
        lines.append(f'  {{ {parent:<28}, {category:<28}, "{descr:<40}", "{comment:<20}", {visible_flag:3} }}, // {descr if descr else comment}')
    lines.append('};\n')
    return '\n'.join(lines)

def generate_selectors(db):
    """Генерирует массивы селекторов selector_X и selectors_list только для используемых в DevParams"""
    selectors = db['Selectors']['rows']
    selectors_lists = db['SelectorsLists']['rows']
    # --- Фильтрация используемых селекторов ---
    used_selectors = set(row[2] for row in db['DevParams']['rows'] if row[2])
    selector_map = {name: i+1 for i, (name, _) in enumerate(selectors) if name in used_selectors}
    # Сначала массивы selector_X
    sel_arrays = []
    filtered_selectors = [(name, descr) for (name, descr) in selectors if name in used_selectors]

    for i, (sel_name, sel_descr) in enumerate(filtered_selectors):
        items = [r for r in selectors_lists if r[0] == sel_name]
        if not items:
            continue
        arr_name = f'selector_{i+1}'
        sel_arrays.append(f'// Selector description:  {sel_descr}')
        sel_arrays.append(f'static const T_selector_items {arr_name}[{len(items)}] = ')
        sel_arrays.append('{')
        for item in items:
            val = item[1]  # ValueStr
            caption = item[2]
            img = item[3]
            pad = max(0, 42 - len(caption))
            sel_arrays.append(f'  {{ {val} , "{caption}"{" " * pad}, {img}}},')
        sel_arrays.append('};\n')
    # Теперь selectors_list только для используемых
    sel_list = []
    sel_list.append(f'static const T_selectors_list selectors_list[{len(filtered_selectors)}] = ')
    sel_list.append('{')
    for i, (sel_name, _) in enumerate(filtered_selectors):
        items = [r for r in selectors_lists if r[0] == sel_name]
        arr_name = f'selector_{i+1}' if items else '0'
        arr_len = len(items) if items else 0
        sel_list.append(f'  {{"{sel_name:<30}", {arr_len:<4}, {arr_name:<12}}},')
    sel_list.append('};\n')
    return '\n'.join(sel_arrays + sel_list)

def generate_arr_wvar(db):
    """Генерирует массив arr_wvar для {ProfileName}_Params.c"""
    lines = []
    devparams = db['DevParams']['rows']
    vartypes = {row[0]: row[1] for row in db['DevVarTypes']['rows']}

    # Используем только используемые селекторы (как в generate_selectors)
    selectors = db['Selectors']['rows']
    used_selectors = set(row[2] for row in db['DevParams']['rows'] if row[2])
    filtered_selectors = [(name, descr) for (name, descr) in selectors if name in used_selectors]
    # Создаем правильный selector_map для соответствия индексам в selectors_list (начиная с 0)
    selector_map = {name: i for i, (name, _) in enumerate(filtered_selectors)}

    # Получаем имена из DevProfiles
    struct_name = db['DevProfiles']['rows'][0][2] if db['DevProfiles']['rows'] else 'wvar'
    wvar_size_name = f'{struct_name.upper()}_SIZE'
    arr_name = f'arr_{struct_name}'

    # Вычисляем максимальные ширины для val и varlen
    max_val_len = 0
    max_varlen_len = 0

    for row in devparams:
        var_name = row[5]
        vartype = row[6]
        val = f'(void*)&{struct_name}.{var_name}'
        varlen = f'sizeof({struct_name}.{var_name})' + (f'-1' if vartype == 'tstring' else '')
        max_val_len = max(max_val_len, len(val))
        max_varlen_len = max(max_varlen_len, len(varlen))

    # Минимальные ширины
    max_val_len = max(max_val_len, 25)
    max_varlen_len = max(max_varlen_len, 25)

    lines.append('// Array items are sorted by CategoryName followed by SubNumber')
    lines.append(f'static const T_NV_parameters {arr_name}[{wvar_size_name}] =')
    lines.append('{')
    lines.append(f'  //         var_name{" " * (40 - 8)}, var_description{" " * (80 - 15)}, var_alias, val{" " * (max_val_len - 3)}, vartype, defval, minval, maxval , attr, parmnlev            , pdefval            , format , func, varlen{" " * (max_varlen_len - 6)}, menu_pos, selector_id')

    for idx, row in enumerate(devparams):
        var_name = row[5]
        var_descr = row[3]
        var_alias = row[4][:8] if row[4] else ''
        val = f'(void*)&{struct_name}.{var_name}'
        vartype = row[6]
        defval = row[7] if row[7] is not None else 0
        minval = row[8] if row[8] is not None else 0
        maxval = row[9] if row[9] is not None else 0
        attr = row[10] if row[10] is not None else 0
        parmnlev = row[0]

        # pdefval должен быть строкой по умолчанию, а не спецификатором формата
        pdefval = f'"{row[11]}"' if row[11] else '""'
        fmt = f'"{row[12]}"' if row[12] else '""'
        func = row[13] if row[13] else 0
        varlen = f'sizeof({struct_name}.{var_name})' + (f'-1' if vartype == 'tstring' else '')
        menu_pos = row[1] if row[1] is not None else 0
        selector_id = selector_map.get(row[2], 0)
        lines.append(f'  {{ "{var_name:<40}", "{var_descr:<80}", "{var_alias:<8}", {val:<{max_val_len}}, "{vartype}", {defval:<5}, {minval:<5}, {maxval:<6}, {attr:<4}, {parmnlev}, {pdefval}, {fmt}, 0, {varlen:<{max_varlen_len}}, {menu_pos:<7}, {selector_id:<2} }}, // /* {idx:02d} */')
    lines.append('};\n')
    return '\n'.join(lines)

def generate_wvar_inst(db):
    """Генерирует структуру const T_NV_parameters_instance wvar_inst"""
    struct_name = db['DevProfiles']['rows'][0][2] if db['DevProfiles']['rows'] else 'wvar'
    wvar_inst_name = f'{struct_name}_inst'
    wvar_size_name = f'{struct_name.upper()}_SIZE'
    arr_name = f'arr_{struct_name}'
    # Считаем только записи где parent != NULL
    n_parmenu = len([row for row in db['DevParamTree']['rows'] if row[1] is not None])
    n_selectors = len(db['Selectors']['rows'])
    return (
        f'const T_NV_parameters_instance {wvar_inst_name} =\n'
        '{\n'
        f'  {wvar_size_name},\n'
        f'  {arr_name},\n'
        f'  {n_parmenu},\n'
        f'  parmenu,\n'
        f'  SELECTORS_NUM,\n'
        f'  selectors_list\n'
        '};\n'
    )

def generate_param_hash_table(db):
    """Генерирует таблицу CRC16 хешей имен параметров для быстрого поиска"""
    lines = []
    devparams = db['DevParams']['rows']

    # Собираем пары (хеш, индекс) для всех параметров
    hash_entries = []
    for i, row in enumerate(devparams):
        var_name = row[5]  # Variable_name в 6-м столбце (индекс 5)
        hash_value = crc16_ccitt(var_name)
        hash_entries.append((hash_value, i, var_name))

    # Сортируем по хешу для использования бинарного поиска
    hash_entries.sort(key=lambda x: x[0])

    lines.append('// Parameter hash table for fast parameter lookup by CRC16 hash')
    lines.append('// Table is sorted by hash values for binary search')
    lines.append('typedef struct')
    lines.append('{')
    lines.append('  uint16_t hash;     // CRC16 hash of parameter name')
    lines.append('  uint16_t index;    // Index in parameter array')
    lines.append('} T_param_hash_entry;')
    lines.append('')

    lines.append('static const T_param_hash_entry param_hash_table[WVAR_SIZE] =')
    lines.append('{')

    for i, (hash_value, param_index, var_name) in enumerate(hash_entries):
        is_last = (i == len(hash_entries) - 1)
        comma = '' if is_last else ','
        lines.append(f'  {{0x{hash_value:04X}, {param_index:2d}}}{comma}  // {var_name}')

    lines.append('};')
    lines.append('')

    # Добавляем функцию поиска
    lines.append('// Binary search function to find parameter index by CRC16 hash')
    lines.append('// Returns parameter index or 0xFFFF if not found')
    lines.append('uint16_t Find_param_by_hash(uint16_t hash)')
    lines.append('{')
    lines.append('  int left = 0;')
    lines.append('  int right = WVAR_SIZE - 1;')
    lines.append('')
    lines.append('  while (left <= right)')
    lines.append('  {')
    lines.append('    int mid = (left + right) / 2;')
    lines.append('    if (param_hash_table[mid].hash == hash)')
    lines.append('    {')
    lines.append('      return param_hash_table[mid].index;')
    lines.append('    }')
    lines.append('    if (param_hash_table[mid].hash < hash)')
    lines.append('    {')
    lines.append('      left = mid + 1;')
    lines.append('    }')
    lines.append('    else')
    lines.append('    {')
    lines.append('      right = mid - 1;')
    lines.append('    }')
    lines.append('  }')
    lines.append('  return 0xFFFF; // Parameter not found')
    lines.append('}')
    lines.append('')

    return '\n'.join(lines)

def generate_param_index_to_hash_table(db):
    """Генерирует таблицу для получения хеша параметра по его индексу (для CAN команд)"""
    lines = []
    devparams = db['DevParams']['rows']

    # Собираем хеши для всех параметров в порядке их индексов
    hash_entries = []
    for i, row in enumerate(devparams):
        var_name = row[5]  # Variable_name в 6-м столбце (индекс 5)
        hash_value = crc16_ccitt(var_name)
        hash_entries.append((i, hash_value, var_name))

    lines.append('// Parameter index-to-hash table for CAN command transmission')
    lines.append('// Array index corresponds to parameter index, value is CRC16 hash')
    lines.append(f'static const uint16_t param_index_to_hash_table[WVAR_SIZE] =')
    lines.append('{')

    for i, (param_index, hash_value, var_name) in enumerate(hash_entries):
        is_last = (i == len(hash_entries) - 1)
        comma = '' if is_last else ','
        lines.append(f'  0x{hash_value:04X}{comma}  // [{param_index:2d}] {var_name}')

    lines.append('};')
    lines.append('')

    # Добавляем функцию получения хеша по индексу
    lines.append('// Function to get parameter hash by index for CAN transmission')
    lines.append('// Returns parameter hash or 0x0000 if index is out of range')
    lines.append('uint16_t Get_param_hash_by_index(uint16_t index)')
    lines.append('{')
    lines.append(f'  if (index >= WVAR_SIZE)')
    lines.append('  {')
    lines.append('    return 0x0000; // Invalid index')
    lines.append('  }')
    lines.append('  return param_index_to_hash_table[index];')
    lines.append('}')
    lines.append('')

    return '\n'.join(lines)

# --- Генерация .c ---
def generate_c(db):
    # Получаем имя профиля из DevProfiles
    profile_name = db['DevProfiles']['rows'][0][0] if db['DevProfiles']['rows'] else 'DEV'

    lines = []
    lines.append('// This file is auto-generated. Do not edit manually.')
    lines.append('#include "App.h"')
    lines.append(f'#include "{profile_name}_Params.h"')
    lines.append('')

    # Макросы WVAR_SIZE и SELECTORS_NUM
    wvar_size = len(db['DevParams']['rows'])
    selectors_num = len(db['Selectors']['rows'])
    struct_name = db['DevProfiles']['rows'][0][2] if db['DevProfiles']['rows'] else 'wvar'
    wvar_size_name = f'{struct_name.upper()}_SIZE'
    lines.append(f'#define {wvar_size_name} {wvar_size}')
    lines.append(f'#define SELECTORS_NUM {selectors_num}')
    lines.append('')

    # Объявление wvar с правильным именем из DevProfiles
    wvar_type = f'{struct_name.upper()}_TYPE'
    lines.append(f'{wvar_type} {struct_name};')
    lines.append('')
    # parmenu
    lines.append(generate_parmenu(db))
    # Parameter hash table
    lines.append(generate_param_hash_table(db))
    # Parameter index-to-hash table
    lines.append(generate_param_index_to_hash_table(db))
    # arr_wvar
    lines.append(generate_arr_wvar(db))
    # Селекторы
    lines.append(generate_selectors(db))
    # wvar_inst
    lines.append(generate_wvar_inst(db))
    lines.append('')
    return '\n'.join(lines)

def postprocess_arr_wvar_c_file(c_path):
    """
    Пост-обработка файла .c: обработка значений в массиве arr_wvar с выравниванием столбцов без изменения содержимого строк.
    """

    with open(c_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # Найти начало и конец массива arr_wvar
    start_idx = None
    end_idx = None
    for i, line in enumerate(lines):
        if re.match(r'.*static const T_NV_parameters.*=\s*$', line):
            start_idx = i
        if start_idx is not None and line.strip().startswith('};'):
            end_idx = i
            break
    if start_idx is None or end_idx is None:
        return  # массив не найден

    arr_lines = lines[start_idx:end_idx+1]
    # Найти строку с заголовками
    header_idx = None
    for i, l in enumerate(arr_lines):
        if 'var_name' in l and 'var_description' in l:
            header_idx = i
            break
    if header_idx is None:
        return
    header_line = arr_lines[header_idx]

    # Получить имена столбцов
    header_cols = [x.strip(' /*,\n') for x in header_line.split(',')]

    # Определить индексы специальных столбцов
    vartype_col_idx = None
    parmnlev_col_idx = None
    pdefval_col_idx = None
    format_col_idx = None

    for i, col in enumerate(header_cols):
        if 'vartype' in col:
            vartype_col_idx = i
        elif 'parmnlev' in col:
            parmnlev_col_idx = i
        elif 'pdefval' in col:
            pdefval_col_idx = i
        elif 'format' in col:
            format_col_idx = i

    # Собрать все строки-элементы массива и комментарии
    data_lines = []
    comments = []
    for l in arr_lines[header_idx+1:]:
        if l.strip().startswith('{'):
            # Сохраняем комментарий к строке (если есть)
            comment = ''
            if '//' in l:
                l, comment = l.split('//', 1)
                comment = '// ' + comment.strip()
            else:
                comment = ''
            # Сохраняем комментарии с номерами строк внутри массива
            data_lines.append(l)
            comments.append(comment)
    # Парсинг значений по запятым, учитывая кавычки
    parsed = []
    for l in data_lines:
        reader = csv.reader(StringIO(l.strip().lstrip('{').rstrip('},')), skipinitialspace=True)
        for row in reader:
            parsed.append(row)

    # Специальная обработка значений в определенных столбцах
    for row in parsed:
        for i, val in enumerate(row):
            val_stripped = val.strip()

            # Обработка столбца pdefval: заменить null и строки заполненные пробелами на пустые строки
            if i == pdefval_col_idx:
                # Если значение пустое, null или только пробелы - заменить на пустую строку
                if not val_stripped or val_stripped.lower() == 'null':
                    row[i] = '""'
                # Если строка состоит только из пробелов (в кавычках или без)
                elif val_stripped.startswith('"') and val_stripped.endswith('"'):
                    inner_val = val_stripped[1:-1]
                    if inner_val == '' or inner_val.isspace():
                        row[i] = '""'
                    else:
                        row[i] = val_stripped  # Оставляем как есть, уже в кавычках
                # Если значение не в кавычках - обернуть в кавычки (для всех строковых значений)
                else:
                    row[i] = f'"{val_stripped}"'

            # Обработка столбца format: убрать лишние пробелы из простых форматеров
            elif i == format_col_idx and val_stripped:
                if val_stripped.startswith('"') and val_stripped.endswith('"'):
                    inner_val = val_stripped[1:-1].strip()
                    row[i] = f'"{inner_val}"'
                elif val_stripped not in ('null', '0'):  # Не трогать null и 0
                    row[i] = f'"{val_stripped.strip()}"'

    # Подготовка данных для выравнивания: обработка значений без изменения содержимого строк
    processed_rows = []
    for row in parsed:
        processed_row = []
        for i, val in enumerate(row):
            val = val.strip()

            # Специальная обработка для столбцов vartype и parmnlev - убираем кавычки (идентификаторы макросов/перечислений)
            if i == vartype_col_idx or i == parmnlev_col_idx:
                clean_val = val.strip('"').strip()
                processed_row.append(clean_val if clean_val else '')
            # Специальная обработка для столбца format - оставляем как есть
            elif i == format_col_idx:
                processed_row.append(val)
            # Специальная обработка для столбца pdefval - оставляем как есть
            elif i == pdefval_col_idx:
                processed_row.append(val)
            # Если это строка (не число, не указатель, не sizeof и не пусто), то обернуть в кавычки
            else:
                if (val == '' or val.startswith('"') and val.endswith('"')):
                    # Уже строка с кавычками или пустая - оставляем как есть
                    processed_row.append(val)
                elif not re.match(r'^[-+]?\d+(\.\d+)?$', val) and not val.startswith('(void*') and not val.startswith('sizeof'):
                    # Не число, не указатель, не sizeof - оборачиваем в кавычки без дополнительных пробелов
                    processed_row.append(f'"{val}"')
                else:
                    processed_row.append(val)
        processed_rows.append(processed_row)

    # Сначала обработать все строки и добавить комментарии с номерами (подготовка финального содержимого)
    final_rows = []
    for idx, row in enumerate(processed_rows):
        final_row = []
        for i, val in enumerate(row):
            if i == 0:  # Первый столбец - добавляем комментарий с номером строки
                comment = comments[idx] if idx < len(comments) else ''
                line_num_match = re.search(r'/\*\s*(\d+)\s*\*/', comment)
                if line_num_match:
                    line_num = line_num_match.group(1)
                    # Убираем кавычки из первого поля и добавляем номер строки
                    if val.startswith('"') and val.endswith('"'):
                        var_name = val[1:-1].strip()
                        final_val = f'/* {line_num} */ "{var_name}"'
                    else:
                        final_val = f'/* {line_num} */ {val}'
                    final_row.append(final_val)
                else:
                    final_row.append(val)
            else:
                final_row.append(val)
        final_rows.append(final_row)

    # Теперь рассчитать максимальные длины для каждого столбца на основе финального содержимого
    col_widths = [len(h) for h in header_cols]
    print(f'[DEBUG] Expected columns: {len(col_widths)}, header: {header_cols}')
    for row in final_rows:
        if len(row) != len(col_widths):
            print(f'[arr_wvar align warn] Skipping row with {len(row)} columns, expected {len(col_widths)}: {row}')
            continue
        for i, val in enumerate(row):
            col_widths[i] = max(col_widths[i], len(val))

    # Переформатировать заголовок с выравниванием
    new_header = '  // ' + ', '.join(f'{h:<{col_widths[i]}}' for i, h in enumerate(header_cols)) + '\n'

    # Переформатировать строки массива с выравниванием
    new_data_lines = []
    for idx, row in enumerate(final_rows):
        if len(row) != len(col_widths):
            continue
        vals = []
        for i, val in enumerate(row):
            # Добавляем пробелы справа для выравнивания столбца
            vals.append(f'{val:<{col_widths[i]}}')

        # Определяем, является ли это последней строкой
        is_last_row = (idx == len(final_rows) - 1)
        comma = '' if is_last_row else ','
        new_data_lines.append(f'  {{ ' + ', '.join(vals) + f' }}{comma} \n')
    # Собрать новый массив
    new_arr = arr_lines[:header_idx] + [new_header] + new_data_lines + [arr_lines[-1]]
    # Заменить в файле
    lines = lines[:start_idx] + new_arr + lines[end_idx+1:]
    with open(c_path, 'w', encoding='utf-8') as f:
        f.writelines(lines)

# --- Главная функция ---
def get_file_paths(db):
    """Генерирует пути к выходным файлам на основе ProfileName"""
    profile_name = db['DevProfiles']['rows'][0][0] if db['DevProfiles']['rows'] else 'MC80'
    base_dir = os.path.dirname(__file__)
    c_path = os.path.join(base_dir, f'{profile_name}_Params.c')
    h_path = os.path.join(base_dir, f'{profile_name}_Params.h')
    return c_path, h_path

def main():
    db = load_params_db()
    params_c_path, params_h_path = get_file_paths(db)
    h = generate_h(db)
    c = generate_c(db)
    with open(params_h_path, 'w', encoding='utf-8') as f:
        f.write(h)
    with open(params_c_path, 'w', encoding='utf-8') as f:
        f.write(c)
    # Пост-обработка выравнивания массива arr_wvar
    postprocess_arr_wvar_c_file(params_c_path)
    print(f'Generated: {params_h_path}\nGenerated: {params_c_path}')

if __name__ == '__main__':
    main()
