import subprocess
import pathlib
from concurrent.futures import ProcessPoolExecutor

root_dir = pathlib.Path("../scripts")
max_workers = 8

def run_root_script(path):
    '''Скрипт для запуска root-файлов без видимого окна по адресу'''
    print(f">>> Запуска файла: {path.name}")

    command = ["root", "-l", "-b", "-q", str(path)]

    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        return f"Успех при обработке файла: {path.name}"
    except subprocess.CalledProcessError as e:
        return f"Ошибка {e} в файле: {path.name}"

def main():
    scripts = list(root_dir.glob("*.C"))
    if not scripts:
        print("Не найдено ни одного скрипта!")
        return
    else:
        print(f"Найдено {len(scripts)} скриптов! Начинаем запуск в {max_workers} потоках")

    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        results = list(executor.map(run_root_script, scripts))

    for res in results:
        print(res)


if __name__ == "__main__":
    main()