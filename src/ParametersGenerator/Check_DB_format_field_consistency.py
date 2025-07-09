#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Скрипт для проверки соответствия поля format полю Variable_type в базе данных параметров
"""

import json
import re
import sys

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

def check_variable_name_hashes(db):
    """
    Проверяет уникальность CRC16 хешей имен переменных

    Args:
        db: словарь с данными базы параметров

    Returns:
        tuple: (is_unique, hash_errors, hash_info)
    """
    dev_params = db.get('DevParams', {})
    columns = dev_params.get('columns', [])
    rows = dev_params.get('rows', [])

    try:
        var_name_idx = columns.index('Variable_name')
        category_idx = columns.index('Category')
        subnumber_idx = columns.index('SubNumber')
    except ValueError as e:
        return False, [f"Не найден обязательный столбец: {e}"], {}

    hash_info = {}  # hash_value -> list of (var_name, category, subnumber)
    hash_errors = []

    for i, row in enumerate(rows):
        if len(row) <= max(var_name_idx, category_idx, subnumber_idx):
            continue

        var_name = row[var_name_idx]
        category = row[category_idx]
        subnumber = row[subnumber_idx]

        # Вычисляем CRC16 хеш для имени переменной
        hash_value = crc16_ccitt(var_name)

        if hash_value not in hash_info:
            hash_info[hash_value] = []

        hash_info[hash_value].append((var_name, category, subnumber))

    # Проверяем на коллизии
    is_unique = True
    for hash_value, params in hash_info.items():
        if len(params) > 1:
            is_unique = False
            param_list = ', '.join([f"'{p[0]}' ({p[1]}.{p[2]})" for p in params])
            hash_errors.append(f"CRC16 hash collision 0x{hash_value:04X}: {param_list}")

    return is_unique, hash_errors, hash_info

def is_valid_format_specifier(format_str, allowed_specifiers):
    """
    Проверяет валидность спецификатора формата с помощью регулярного выражения

    Args:
        format_str: строка формата (например, "%.2f", "%10d", "%s", "%ld")
        allowed_specifiers: список разрешенных спецификаторов (например, ['f', 'F', 'e', 'E', 'g', 'G'])

    Returns:
        tuple: (is_valid, error_message)
    """
    if not format_str.startswith('%'):
        return False, "Format specifier must start with '%'"

    # Регулярное выражение для разбора спецификатора формата
    # %[flags][width][.precision][length]specifier
    # length может быть: l, ll, h, hh (например %ld, %lld)
    pattern = r'^%([+\-# 0]*)(\d*)(?:\.(\d+))?([lh]*)([a-zA-Z])$'

    match = re.match(pattern, format_str)
    if not match:
        return False, f"Invalid format specifier syntax: '{format_str}'"

    flags, width, precision, length_modifier, specifier = match.groups()

    # Собираем полный спецификатор с модификатором длины
    full_specifier = (length_modifier or '') + specifier

    # Проверяем, что спецификатор разрешен
    if full_specifier not in allowed_specifiers and specifier not in allowed_specifiers:
        return False, f"Invalid specifier '{full_specifier}' in '{format_str}'. Allowed: {allowed_specifiers}"

    # Дополнительные проверки для конкретных спецификаторов
    if specifier in ['f', 'F', 'e', 'E', 'g', 'G', 'a', 'A']:
        # Для float-форматов precision должна быть разумной
        if precision and int(precision) > 20:
            return False, f"Precision too high ({precision}) in '{format_str}'"

    if specifier in ['d', 'i', 'o', 'u', 'x', 'X']:
        # Для целочисленных форматов precision обычно не используется
        if precision:
            return False, f"Precision not applicable for integer format '{format_str}'"

    return True, ""

def check_format_for_type(format_str, variable_type):
    """
    Проверяет соответствие формата типу переменной

    Args:
        format_str: строка формата
        variable_type: тип переменной

    Returns:
        tuple: (is_valid, is_warning, error_message)
    """
    # Определяем разрешенные спецификаторы для каждого типа
    type_specifiers = {
        'tint8u':       ['d', 'u', 'x', 'X', 'o', 'i'],          # uint8_t
        'tint16u':      ['d', 'u', 'x', 'X', 'o', 'i'],          # uint16_t
        'tint32u':      ['d', 'u', 'x', 'X', 'o', 'i', 'ld', 'lu'], # uint32_t
        'tint32s':      ['d', 'i', 'ld'],                        # int32_t
        'tfloat':       ['f', 'F', 'e', 'E', 'g', 'G', 'a', 'A'], # float
        'tstring':      ['s'],                                   # string
        'tarrofbyte':   ['s', 'd', 'x', 'X'],                   # byte array
        'tarrofdouble': ['f', 'F', 'e', 'E', 'g', 'G', 'a', 'A'] # float array
    }

    if variable_type not in type_specifiers:
        return False, False, f"Unknown variable type '{variable_type}'"

    allowed_specifiers = type_specifiers[variable_type]
    is_valid, error_msg = is_valid_format_specifier(format_str, allowed_specifiers)

    if not is_valid:
        return False, False, error_msg

    # Проверки для предупреждений
    if variable_type in ['tint8u', 'tint16u', 'tint32u'] and format_str == '%s':
        return True, True, f"Using string format '%s' for numeric type '{variable_type}' - possible error"

    if variable_type == 'tstring' and re.search(r'%.*[diouxX]', format_str):
        return True, True, f"Using numeric format for string type '{variable_type}' - possible error"

    if variable_type == 'tfloat' and re.search(r'%.*[diouxX]', format_str):
        return True, True, f"Using integer format for float type '{variable_type}' - possible error"

    return True, False, ""

def check_format_type_consistency(db_file):
    """
    Проверяет соответствие поля format полю Variable_type в базе данных параметров
    и уникальность CRC16 хешей имен переменных

    Returns:
        tuple: (is_consistent, error_list, warning_list, summary)
    """

    # Инициализируем summary сразу
    summary = {
        'total_params': 0,
        'format_errors': 0,
        'format_warnings': 0,
        'hash_errors': 0,
        'type_distribution': {},
        'format_distribution': {},
        'hash_collisions': 0
    }# Загружаем базу данных
    try:
        with open(db_file, 'r', encoding='utf-8') as f:
            db = json.load(f)
    except Exception as e:
        return False, [f"Ошибка чтения файла {db_file}: {e}"], [], summary

    errors = []
    warnings = []

    # Проверяем параметры
    dev_params = db.get('DevParams', {})
    columns = dev_params.get('columns', [])
    rows = dev_params.get('rows', [])

    # Находим индексы нужных столбцов
    try:
        var_type_idx = columns.index('Variable_type')
        format_idx = columns.index('format')
        var_name_idx = columns.index('Variable_name')
        category_idx = columns.index('Category')
        subnumber_idx = columns.index('SubNumber')
    except ValueError as e:
        return False, [f"Не найден обязательный столбец: {e}"], [], summary

    summary['total_params'] = len(rows)

    for i, row in enumerate(rows):
        if i >= len(rows) or len(row) <= max(var_type_idx, format_idx, var_name_idx):
            errors.append(f"Строка {i+1}: недостаточно столбцов")
            continue

        variable_type = row[var_type_idx]
        format_str = row[format_idx]
        var_name = row[var_name_idx]
        category = row[category_idx]
        subnumber = row[subnumber_idx]

        # Статистика по типам и форматам
        summary['type_distribution'][variable_type] = summary['type_distribution'].get(variable_type, 0) + 1
        summary['format_distribution'][format_str] = summary['format_distribution'].get(format_str, 0) + 1

        param_info = f"Параметр '{var_name}' ({category}.{subnumber})"        # Используем новую гибкую проверку форматов
        is_valid, is_warning, error_msg = check_format_for_type(format_str, variable_type)

        if not is_valid:
            errors.append(f"{param_info}: {error_msg}")
            summary['format_errors'] += 1
        elif is_warning:
            warnings.append(f"{param_info}: {error_msg}")
            summary['format_warnings'] += 1

    # Проверяем уникальность CRC16 хешей имен переменных
    is_hash_unique, hash_errors, hash_info = check_variable_name_hashes(db)

    if not is_hash_unique:
        errors.extend(hash_errors)
        summary['hash_errors'] = len(hash_errors)
        summary['hash_collisions'] = len([h for h in hash_info.values() if len(h) > 1])

    is_consistent = len(errors) == 0

    return is_consistent, errors, warnings, summary, hash_info

def print_results(is_consistent, errors, warnings, summary, hash_info=None):
    """Выводит результаты проверки"""

    print("=" * 80)
    print("АНАЛИЗ СООТВЕТСТВИЯ ПОЛЯ 'format' ПОЛЮ 'Variable_type'")
    print("И ПРОВЕРКА УНИКАЛЬНОСТИ CRC16 ХЕШЕЙ ИМЕН ПЕРЕМЕННЫХ")
    print("=" * 80)
    print()

    print(f"Всего параметров проверено: {summary['total_params']}")
    print(f"Ошибок форматов: {summary['format_errors']}")
    print(f"Предупреждений форматов: {summary['format_warnings']}")
    if 'hash_errors' in summary:
        print(f"Коллизий CRC16 хешей: {summary.get('hash_collisions', 0)}")
    print()

    if is_consistent and len(warnings) == 0:
        print("✅ ВСЕ ПОЛЯ СООТВЕТСТВУЮТ ДРУГ ДРУГУ И ХЕШИ УНИКАЛЬНЫ!")
    elif len(errors) == 0:
        print("⚠️  КРИТИЧЕСКИХ ОШИБОК НЕТ, НО ЕСТЬ ПРЕДУПРЕЖДЕНИЯ")
    else:
        print("❌ ОБНАРУЖЕНЫ КРИТИЧЕСКИЕ ОШИБКИ!")

    print()

    # Статистика по типам
    print("РАСПРЕДЕЛЕНИЕ ПО ТИПАМ ПЕРЕМЕННЫХ:")
    print("-" * 40)
    for var_type, count in sorted(summary['type_distribution'].items()):
        print(f"  {var_type:<15}: {count:>3} параметров")
    print()    # Статистика по форматам
    print("РАСПРЕДЕЛЕНИЕ ПО ФОРМАТАМ:")
    print("-" * 40)
    for format_str, count in sorted(summary['format_distribution'].items()):
        print(f"  {format_str:<15}: {count:>3} параметров")
    print()

    # Информация о CRC16 хешах
    if hash_info:
        print("CRC16 ХЕШИ ИМЕН ПЕРЕМЕННЫХ:")
        print("-" * 80)
        sorted_hashes = sorted(hash_info.items())
        for hash_value, params in sorted_hashes:
            if len(params) == 1:
                var_name, category, subnumber = params[0]
                print(f"  0x{hash_value:04X}: {var_name:<25} ({category}.{subnumber})")
            else:
                # Коллизия - выделим ее
                print(f"  0x{hash_value:04X}: *** КОЛЛИЗИЯ ***")
                for var_name, category, subnumber in params:
                    print(f"         {var_name:<25} ({category}.{subnumber})")
        print()

    # Вывод ошибок
    if errors:
        print("КРИТИЧЕСКИЕ ОШИБКИ:")
        print("-" * 40)
        for i, error in enumerate(errors, 1):
            print(f"{i:>3}. {error}")
        print()

    # Вывод предупреждений
    if warnings:
        print("ПРЕДУПРЕЖДЕНИЯ:")
        print("-" * 40)
        for i, warning in enumerate(warnings, 1):
            print(f"{i:>3}. {warning}")
        print()

    return is_consistent and len(warnings) == 0

def main():
    """Главная функция"""

    db_file = "ParamsDB.txt"

    print("Проверка соответствия полей format и Variable_type...")
    print("и уникальности CRC16 хешей имен переменных...")
    print(f"Файл базы данных: {db_file}")
    print()

    is_consistent, errors, warnings, summary, hash_info = check_format_type_consistency(db_file)

    success = print_results(is_consistent, errors, warnings, summary, hash_info)

    # Пауза для просмотра результатов
    print()
    input("Нажмите Enter для завершения...")

    if not success:
        sys.exit(1)

if __name__ == "__main__":
    main()
