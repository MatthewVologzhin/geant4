# 1. Список энергий (МэВ)
energy_list = [0.0010, 0.0015, 0.0020, 0.0030, 0.0040, 0.0050, 0.0060, 0.0080, 0.0100, 
               0.0150, 0.0200, 0.0300, 0.0400, 0.0500, 0.0600, 0.0800, 0.1000, 0.1500, 
               0.2000, 0.3000, 0.4000, 0.5000, 0.6000, 0.8000, 1.0000, 1.5000, 2.0000, 
               3.0000, 4.0000, 5.0000, 6.0000, 8.0000, 10.0000, 15.0000, 20.0000, 30.0000, 
               40.0000, 50.0000, 60.0000, 80.0000, 100.0000, 150.0000, 200.0000, 300.0000, 
               400.0000, 500.0000, 600.0000, 800.0000, 1000.0000]

# 2. Физические листы (ровно те имена, которые ожидает твое приложение)
physics_lists = ["dna_opt2", "dna_opt4", "dna_opt6", "dna_opt8", "std_opt4"]

# 3. Список частиц и их параметры (Z, A, Q)
particles_config = [
    ("e-", 0, 0, 0),
    ("e+", 0, 0, 0),
    ("proton", 0, 0, 0),
    ("alpha", 0, 0, 0),
    ("ion", 6, 12, 6) # Углерод
]

def get_prts_nb(energy, particle):
    if particle in ["e-", "e+"]: return 1000
    if energy < 0.1: return 5000
    elif energy < 10: return 1000
    elif energy < 100: return 100
    else: return 10

def estimate_resources(phys, particle):
    # Углерод прожорлив - даем 10GB (запас под 9GB), остальным меньше
    if particle == "ion":
        return 1, "10GB"
    elif "dna" in phys:
        return 1, "4GB"
    else:
        return 1, "2GB"

with open("parameters.txt", "w") as f:
    for phys in physics_lists:
        for p_name, z, a, q in particles_config:
            for energy in energy_list:
                beam_on = get_prts_nb(energy, p_name)
                cpus, memory = estimate_resources(phys, p_name)
                
                # Формируем строку: phys, part, z, a, q, en, unit, beam, cpus, mem
                line = f"{phys}, {p_name}, {z}, {a}, {q}, {energy}, MeV, {beam_on}, {cpus}, {memory}\n"
                f.write(line)

print("parameters.txt готов.")