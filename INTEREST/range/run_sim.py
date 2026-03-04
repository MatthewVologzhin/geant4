import os
import subprocess
import time

executable = "./range"
macrosdir = "macros"
os.makedirs(macrosdir, exist_ok=True)

physics_list = ["dna_opt2", "dna_opt4", "dna_opt6", "dna_opt8", "std_opt4"]
particles_list = ["e-", "e+", "proton", "alpha", "C12"]
energy_list = [0.0010, 0.0015, 0.0020, 0.0030, 0.0040, 0.0050, 0.0060, 0.0080, 0.0100, 
    0.0150, 0.0200, 0.0300, 0.0400, 0.0500, 0.0600, 0.0800, 0.1000, 0.1500, 
    0.2000, 0.3000, 0.4000, 0.5000, 0.6000, 0.8000, 1.0000, 1.5000, 2.0000, 
    3.0000, 4.0000, 5.0000, 6.0000, 8.0000, 10.0000, 15.0000, 20.0000, 30.0000, 
    40.0000, 50.0000, 60.0000, 80.0000, 100.0000, 150.0000, 200.0000, 300.0000, 
    400.0000, 500.0000, 600.0000, 800.0000, 1000.0000]
energy_list.reverse()
particles_list.reverse()

def get_prts_nb(energy, particle):
    """Получение числа частиц из энергии"""
    if (particle == "e-" or particle == "e+"):
        return 1000
    if energy < 0.1:        # до 100 кэВ
        return 5000
    elif energy < 10:       # от 100 кэВ до 10 МэВ
        return 1000
    elif energy < 100:      # от 10 МэВ до 100 МэВ
        return 100
    elif energy < 300:      # выше 100 МэВ (включая 1 ГэВ)
        return 10
    else:
        return 10

def get_threads(particle):
    if (particle == "e-" or particle == "e+"): return 6
    else: return 1

def run_simulation():
    for phys in physics_list:
        for part in particles_list:
            
            macros_name = f'{part}_{phys}'
            macros_path = os.path.join(macrosdir, f"{macros_name}.mac")

            with open(macros_path, "w") as f:
                threads = get_threads(part)
                f.write(f"/run/numberOfThreads {threads}\n")
                f.write(f"/range/phys/addPhysics {phys}\n")
                f.write("/range/det/setTrackingCut 10 eV\n")
                f.write("/range/det/setMat G4_WATER\n")
                f.write("/range/det/setRadius 10 m\n")
                f.write("/tracking/storeTrajectory 0\n")
                f.write("/control/verbose 0\n")
                f.write("/run/verbose 0\n")
                f.write("/event/verbose 0\n")
                f.write("/tracking/verbose 0\n")

                #f.write("/analysis/verbose 0\n")

                f.write("/process/verbose 0\n")
                f.write("/process/em/verbose 0\n")
                f.write("/process/had/verbose 0\n")

                f.write("/run/initialize\n")

                if (part == "C12"):
                    f.write("/gun/particle ion\n")
                    f.write("/gun/ion 6 12 6\n")
                else:
                    f.write(f"/gun/particle {part}\n")
                
                for en in energy_list:
                    particles_nb = get_prts_nb(en, part)
                    f.write(f"/gun/energy {en} MeV\n")
                    f.write(f"/run/beamOn {particles_nb}\n")
                
            print(f"=== [START] Physics: {phys} | Particle: {part}")
            try:
                subprocess.run([executable, macros_path], check=True)
            except subprocess.CalledProcessError:
                print(f"!!! Error: Simulation failed of {macros_name}")
                time.sleep(20)
                continue
            except Exception as e:
                print(f"!!! Fatal: {e}")
                time.sleep(20)
                continue

if __name__ == "__main__":
    run_simulation()