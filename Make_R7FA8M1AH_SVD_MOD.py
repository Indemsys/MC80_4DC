import re
import xml.etree.ElementTree as ET

def replace_iel_names(vector_data_path, svd_path, svd_out_path):
    print("Starting replace_iel_names()")
    event_map = {}
    print(f"Reading vector_data from: {vector_data_path}")    # Шаг 1: Читаем таблицу g_interrupt_event_link_select из vector_data.c
    with open(vector_data_path, 'r', encoding='utf-8') as f:
        for line in f:
            # Ищем строки формата: [n] = ELC_EVENT_xxx, где xxx - имя события
            match = re.search(r'^\s*\[(\d+)\]\s*=\s*ELC_EVENT_([A-Za-z0-9_]+)', line)
            if match:
                index = int(match.group(1))
                event_name = match.group(2)
                event_map[index] = event_name
                print(f"Found event for index {index}: {event_name}")

    # Шаг 2: Парсим SVD-файл и меняем IELx на события
    print(f"Parsing XML from: {svd_path}")
    tree = ET.parse(svd_path)
    root = tree.getroot()

    changed_count = 0
    peripherals = root.find('peripherals')
    if peripherals is not None:
        for peripheral in peripherals.findall('peripheral'):
            name_tag = peripheral.find('name')
            if name_tag is not None and name_tag.text.strip() == 'ICU':
                print("Found peripheral: ICU")
                for interrupt in peripheral.findall('interrupt'):
                    intr_name = interrupt.find('name')
                    if intr_name is not None and intr_name.text.startswith('IEL'):
                        try:
                            iel_index = int(intr_name.text[3:])
                            if iel_index in event_map:
                                print(f"Replacing {intr_name.text} with {event_map[iel_index]}")
                                intr_name.text = event_map[iel_index]
                                changed_count += 1
                        except ValueError:
                            print(f"Skipping non-integer interrupt name: {intr_name.text}")

    # Шаг 3: Сохраняем результат в новый файл
    print(f"Writing modified SVD to {svd_out_path}")
    tree.write(svd_out_path, encoding='utf-8', xml_declaration=True)
    print(f"Finished. Total replaced entries: {changed_count}")

# Пример вызова:
replace_iel_names(
    r".\ra_gen\vector_data.c",
    r"R7FA8M1AH.svd",
    r"R7FA8M1AH_mod.svd"
)
