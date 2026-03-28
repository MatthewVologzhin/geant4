import pathlib

input_dir = pathlib.Path("./root")

particles = ["e-", "e+", "proton", "alpha", "C12"]
physics = ["DNA2", "DNA4", "DNA6", "DNA8", "S4"]

for part in particles:
    for phys in physics:
        lines = list()
        output_file = pathlib.Path(f"./root/{part}_{phys}.txt")
        with output_file.open("w", encoding="utf-8") as output:
            for file in sorted(input_dir.glob(f"*{part}_{phys}.txt")):
                content = file.read_text(encoding="utf-8")
                if content:
                    lines.append(content)
            
            lines.sort(key=lambda line: float(line.split()[0]))
            for line in lines:
                output.write(line)

