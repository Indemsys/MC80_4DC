import os
import xml.etree.ElementTree as ET

prog_file_name = "MC80_4DC_Proj.ewp"
proj_dir = os.getcwd()  # Файл должен находиться в корневой директории проекта

excluded_dirs = [
    ".",  # Текущая директория
    ".vscode",  # Конфигурационные файлы редактора VSCode
    ".git",  # Репозиторий Git
    ".settings",  # Дополнительные настройки
    "settings",  # Общая директория настроек
    "Out",  # Директория выходных файлов
    "debug",  # Директория отладочной сборки
    "release",  # Директория релизной сборки
    "script",  # Скрипты
    "scripts",  # Альтернативная директория скриптов
    "xml",  # Директория с XML-файлами
]

included_extensions = {'.c', '.h', '.s', '.S', '.a', '.cpp', '.hpp'}

# ------------------------------------------------------------------------------------------------------------
# Функция рекурсивного прохода вглубь по директориям с корнем заданным в аргументе root_path и создание субблоков в дереве xml_tree
# ------------------------------------------------------------------------------------------------------------
def Create_IAR_groups_tree(current_path, xml_tree):
    n = 0

    # Создаем группу
    iar_group = ET.SubElement(xml_tree, 'group')  # Создаем XML тэг group в дереве переданном в качестве аргумента xml_tree - это будет блок содержащий элементы каталога в дереве IAR
    name_tag = ET.SubElement(iar_group, 'name')  # В тэге group создаем тэг name - это будет именем группы в дереве каталогов IAR
    name_tag.text = os.path.basename(current_path)  # Получаем полный путь к текущей директории

    # Рекурсивно проходим по дереву директорий
    dirs_iterator = os.scandir(current_path)
    for directory_object in dirs_iterator:
        deeper_path = os.path.join(current_path, directory_object.name)

        # Проверка на наличие директории в списке исключений
        if (not any(excl_dir == directory_object.name for excl_dir in excluded_dirs)
                and not directory_object.name.startswith('.')
                and directory_object.is_dir()):

            print('Group: ' + deeper_path)
            n = n + Create_IAR_groups_tree(deeper_path, iar_group)  # Рекурсивно уходим в глубь дерева директорий

    # Обрабатываем файлы в текущей директории
    dirs_iterator = os.scandir(current_path)
    for directory_object in dirs_iterator:
        if not directory_object.name.startswith('.') and directory_object.is_file():
            fname = os.path.join(current_path, directory_object.name)

            # Проверяем, заканчивается ли имя файла на разрешенное расширение
            if any(fname.endswith(ext) for ext in included_extensions):
                p = fname.replace(proj_dir, "$PROJ_DIR$")
                file_tag = ET.SubElement(iar_group, 'file')  # Вставляем тэг file в текущее дерево группы
                name_tag = ET.SubElement(file_tag, 'name')  # Вставляем в блок тэга file блок name с именем файла
                name_tag.text = p
                n = n + 1
                print('File:  ' + p)

    if n == 0:
        xml_tree.remove(iar_group)  # Если не было записано ни одного файла, то уже вставленный каталог удаляем

    ET.indent(iar_group, space=" ", level=0)
    return n

# ------------------------------------------------------------------------------------------------------------
# Функция для сбора всех директорий на пути к .h файлам
# ------------------------------------------------------------------------------------------------------------
def collect_include_paths_with_h_files(root_path, excluded_dirs):
    """
    Собирает все директории, которые содержат файлы .h, включая промежуточные пути.
    """
    include_paths = set()
    for dirpath, dirs, files in os.walk(root_path, followlinks=True):
        # Пропуск директорий из списка исключений
        if any(os.path.normpath(excl_dir) in os.path.normpath(dirpath).split(os.sep) for excl_dir in excluded_dirs):
            continue

        # Если в директории есть .h файлы, добавляем все промежуточные пути
        if any(file.endswith(".h") for file in files):
            relative_path = os.path.relpath(dirpath, root_path)
            parts = relative_path.split(os.sep)
            for i in range(1, len(parts) + 1):
                intermediate_path = os.path.join(root_path, *parts[:i])
                normalized_path = intermediate_path.replace(proj_dir, "$PROJ_DIR$")
                include_paths.add(normalized_path)

    return sorted(include_paths)

# ------------------------------------------------------------------------------------------------------------
# Начало выполнения программы
# ------------------------------------------------------------------------------------------------------------

full_proj_name = os.path.join(proj_dir, prog_file_name)  # Получаем полный путь к файлу проекта IAR

# Открываем файл проекта
try:
    tree = ET.parse(full_proj_name)  # Парсим XML файл проекта IAR
    xml_root = tree.getroot()
except FileNotFoundError:
    print(f"Error: Project file {prog_file_name} not found in {proj_dir}")
    exit(1)

XMLblock_configuration = xml_root.findall('configuration')  # Находим блок с конфигурацией

# Итерация по всем интересующим блокам где могут находиться объявления путей поиска .h файлов
for XMLblock_configuration_item in XMLblock_configuration:

    for XMLblock_settings in XMLblock_configuration_item.iter('settings'):
        XMLblock_name = XMLblock_settings.find('name')

        # Находим блок содержащий все опции текущей конфигурации
        if XMLblock_name.text == 'ICCARM':
            XMLblock_data = XMLblock_settings.find('data')
            break

    # Удаляем старый блок CCIncludePath2
    for XMLblock_option in XMLblock_data.iter('option'):
        XMLblock_option_name = XMLblock_option.find('name')
        if XMLblock_option_name.text == 'CCIncludePath2':
            XMLblock_data.remove(XMLblock_option)
            break

    # Создаем новый блок <option> с перечислением путей
    new_XMLblock_option = ET.SubElement(XMLblock_data, 'option')
    new_XMLblock_name = ET.SubElement(new_XMLblock_option, 'name')
    new_XMLblock_name.text = 'CCIncludePath2'

    # Собираем пути включения, включая промежуточные директории
    include_paths = collect_include_paths_with_h_files(proj_dir, excluded_dirs)

    # Добавляем пути в секцию CCIncludePath2
    for path in include_paths:
        new_incl = ET.SubElement(new_XMLblock_option, 'state')
        new_incl.text = path
        print(f'Include: {path}')

    ET.indent(new_XMLblock_option, space=" ", level=0)

# Удаление старых групп и создание новых
block_group = xml_root.findall('group')
for block_group_item in block_group:
    xml_root.remove(block_group_item)

# Генерация групп и файлов
Create_IAR_groups_tree(proj_dir, xml_root)

# Сохранение изменений в проекте
print("Saving updated project file...")
tree.write(full_proj_name, method="xml", xml_declaration=True, short_empty_elements=False, encoding="UTF-8")
print("END!")
