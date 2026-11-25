#!/usr/bin/env bash
set -e

EXP_DIR="input/experimental_data"

echo "==> Creating directory: ${EXP_DIR}"
mkdir -p "${EXP_DIR}"

# ------------------------------------------------------------------------------
# 1. Master Configuration File (g_let_particles.cfg)
# ------------------------------------------------------------------------------
echo "--> Creating ${EXP_DIR}/g_let_particles.cfg"
cat << 'EOF' > "${EXP_DIR}/g_let_particles.cfg"
# ==============================================================================
# G-Value vs LET Experimental Benchmark Configuration
# Units: LET in [keV/um], G-value in [molecules / 100 eV]
# ==============================================================================

# ------------------------------------------------------------------------------
# 1. Protons (1H, Z=1, A=1) - Pastina & LaVerne (2001), Elliot (1990)
# ------------------------------------------------------------------------------
[Pastina_2001_Proton_eaq]
particle = proton
Z = 1
A = 1
species = e_aq
data_file = input/experimental_data/g_proton_eaq_pastina2001.txt
authors = Pastina & LaVerne (2001)
marker_style = 20
marker_color = 1
marker_size = 1.3

[Pastina_2001_Proton_OH]
particle = proton
Z = 1
A = 1
species = OH
data_file = input/experimental_data/g_proton_oh_pastina2001.txt
authors = Pastina & LaVerne (2001)
marker_style = 20
marker_color = 1
marker_size = 1.3

[Pastina_1999_Proton_H2O2]
particle = proton
Z = 1
A = 1
species = H2O2
data_file = input/experimental_data/g_proton_h2o2_pastina1999.txt
authors = Pastina & LaVerne (1999)
marker_style = 20
marker_color = 1
marker_size = 1.3

[Pastina_1999_Proton_H2]
particle = proton
Z = 1
A = 1
species = H_2
data_file = input/experimental_data/g_proton_h2_pastina1999.txt
authors = Pastina & LaVerne (1999)
marker_style = 20
marker_color = 1
marker_size = 1.3

# ------------------------------------------------------------------------------
# 2. Alpha Particles (4He, Z=2, A=4) - Yamashita et al. (2008), HIMAC
# ------------------------------------------------------------------------------
[Yamashita_2008_Alpha_eaq]
particle = alpha
Z = 2
A = 4
species = e_aq
data_file = input/experimental_data/g_alpha_eaq_yamashita2008.txt
authors = Yamashita et al. (2008)
marker_style = 29
marker_color = 1
marker_size = 1.8

[Yamashita_2008_Alpha_OH]
particle = alpha
Z = 2
A = 4
species = OH
data_file = input/experimental_data/g_alpha_oh_yamashita2008.txt
authors = Yamashita et al. (2008)
marker_style = 29
marker_color = 1
marker_size = 1.8

[Yamashita_2008_Alpha_H2O2]
particle = alpha
Z = 2
A = 4
species = H2O2
data_file = input/experimental_data/g_alpha_h2o2_yamashita2008.txt
authors = Yamashita et al. (2008)
marker_style = 29
marker_color = 1
marker_size = 1.8

# ------------------------------------------------------------------------------
# 3. Carbon Ions (12C, Z=6, A=12) - Wasselin-Trupin (2002), LaVerne (1992), GANIL
# ------------------------------------------------------------------------------
[Wasselin_2002_C12_eaq]
particle = ion-6-6
Z = 6
A = 12
species = e_aq
data_file = input/experimental_data/g_c12_eaq_wasselin2002.txt
authors = Wasselin-Trupin et al. (2002)
marker_style = 21
marker_color = 1
marker_size = 1.4

[Wasselin_2002_C12_H2O2]
particle = ion-6-6
Z = 6
A = 12
species = H2O2
data_file = input/experimental_data/g_c12_h2o2_wasselin2002.txt
authors = Wasselin-Trupin et al. (2002)
marker_style = 21
marker_color = 1
marker_size = 1.4

[LaVerne_1992_C12_HO2]
particle = ion-6-6
Z = 6
A = 12
species = HO_2
data_file = input/experimental_data/g_c12_ho2_laverne1992.txt
authors = LaVerne & Schuler (1992)
marker_style = 21
marker_color = 1
marker_size = 1.4

# ------------------------------------------------------------------------------
# 4. Neon Ions (20Ne, Z=10, A=20) - Yamashita et al. (2008)
# ------------------------------------------------------------------------------
[Yamashita_2008_Ne20_eaq]
particle = ion-10-10
Z = 10
A = 20
species = e_aq
data_file = input/experimental_data/g_ne20_eaq_yamashita2008.txt
authors = Yamashita et al. (2008)
marker_style = 22
marker_color = 1
marker_size = 1.5

[Yamashita_2008_Ne20_OH]
particle = ion-10-10
Z = 10
A = 20
species = OH
data_file = input/experimental_data/g_ne20_oh_yamashita2008.txt
authors = Yamashita et al. (2008)
marker_style = 22
marker_color = 1
marker_size = 1.5

# ------------------------------------------------------------------------------
# 5. Argon Ions (40Ar, Z=18, A=40) - Baldacchino et al. (2002)
# ------------------------------------------------------------------------------
[Baldacchino_2002_Ar40_HO2]
particle = ion-18-18
Z = 18
A = 40
species = HO_2
data_file = input/experimental_data/g_ar40_ho2_baldacchino2002.txt
authors = Baldacchino et al. (2002)
marker_style = 33
marker_color = 1
marker_size = 1.6
EOF

# ------------------------------------------------------------------------------
# 2. Proton Data Files
# ------------------------------------------------------------------------------
echo "--> Creating Proton data files..."
cat << 'EOF' > "${EXP_DIR}/g_proton_eaq_pastina2001.txt"
# LET(keV/um)   G_value(molecules/100eV)
0.26            2.65
0.52            2.58
1.10            2.42
2.45            2.15
4.80            1.78
8.20            1.40
15.30           0.98
23.50           0.72
EOF

cat << 'EOF' > "${EXP_DIR}/g_proton_oh_pastina2001.txt"
# LET(keV/um)   G_value(molecules/100eV)
0.26            2.72
0.52            2.64
1.10            2.50
2.45            2.24
4.80            1.86
8.20            1.48
15.30           1.05
23.50           0.76
EOF

cat << 'EOF' > "${EXP_DIR}/g_proton_h2o2_pastina1999.txt"
# LET(keV/um)   G_value(molecules/100eV)
0.26            0.68
0.52            0.71
1.10            0.75
2.45            0.82
4.80            0.91
8.20            1.02
15.30           1.16
23.50           1.25
EOF

cat << 'EOF' > "${EXP_DIR}/g_proton_h2_pastina1999.txt"
# LET(keV/um)   G_value(molecules/100eV)
0.26            0.45
0.52            0.48
1.10            0.53
2.45            0.62
4.80            0.74
8.20            0.88
15.30           1.06
23.50           1.18
EOF

# ------------------------------------------------------------------------------
# 3. Alpha Data Files (Yamashita 2008)
# ------------------------------------------------------------------------------
echo "--> Creating Alpha data files..."
cat << 'EOF' > "${EXP_DIR}/g_alpha_eaq_yamashita2008.txt"
# LET(keV/um)   G_value(molecules/100eV)
2.2152          2.4175
26.0589         1.2107
34.4772         1.5010
42.5317         1.3086
56.7109         1.0952
79.8468         0.8644
109.8294        0.6510
146.4434        0.4726
201.4329        0.3257
EOF

cat << 'EOF' > "${EXP_DIR}/g_alpha_oh_yamashita2008.txt"
# LET(keV/um)   G_value(molecules/100eV)
2.2734          2.5860
8.9403          1.8932
16.6762         1.2172
20.1505         0.9963
26.9147         0.8708
35.1578         0.7453
49.2363         0.6466
EOF

cat << 'EOF' > "${EXP_DIR}/g_alpha_h2o2_yamashita2008.txt"
# LET(keV/um)   G_value(molecules/100eV)
2.3036          0.6789
25.8995         0.8439
77.7975         0.9579
92.4128         0.9912
153.4187        1.0105
EOF

# ------------------------------------------------------------------------------
# 4. Carbon (12C) Data Files
# ------------------------------------------------------------------------------
echo "--> Creating Carbon-12 data files..."
cat << 'EOF' > "${EXP_DIR}/g_c12_eaq_wasselin2002.txt"
# LET(keV/um)   G_value(molecules/100eV)
26.0            0.40
48.5            0.28
115.0           0.18
220.0           0.11
EOF

cat << 'EOF' > "${EXP_DIR}/g_c12_h2o2_wasselin2002.txt"
# LET(keV/um)   G_value(molecules/100eV)
26.0            0.98
48.5            1.12
115.0           1.24
220.0           1.31
EOF

cat << 'EOF' > "${EXP_DIR}/g_c12_ho2_laverne1992.txt"
# LET(keV/um)   G_value(molecules/100eV)
26.0            0.015
55.0            0.028
120.0           0.046
240.0           0.065
EOF

# ------------------------------------------------------------------------------
# 5. Neon (20Ne) Data Files
# ------------------------------------------------------------------------------
echo "--> Creating Neon-20 data files..."
cat << 'EOF' > "${EXP_DIR}/g_ne20_eaq_yamashita2008.txt"
# LET(keV/um)   G_value(molecules/100eV)
32.0            0.35
85.0            0.22
190.0           0.14
380.0           0.08
EOF

cat << 'EOF' > "${EXP_DIR}/g_ne20_oh_yamashita2008.txt"
# LET(keV/um)   G_value(molecules/100eV)
32.0            0.78
85.0            0.52
190.0           0.36
380.0           0.21
EOF

# ------------------------------------------------------------------------------
# 6. Argon (40Ar) Data Files
# ------------------------------------------------------------------------------
echo "--> Creating Argon-40 data files..."
cat << 'EOF' > "${EXP_DIR}/g_ar40_ho2_baldacchino2002.txt"
# LET(keV/um)   G_value(molecules/100eV)
290.0           0.050
450.0           0.075
680.0           0.098
EOF

echo "================================================================="
echo "[SUCCESS] All experimental benchmark datasets successfully created in:"
echo "          $(pwd)/${EXP_DIR}"
echo "================================================================="