# visualize.py
import numpy as np
import pandas as pd

import plotly.graph_objects as go
from plotly.subplots import make_subplots

from config import *

def visualize_molecules(output_name=BASE_DIR / "results" / "plots" / "Macromolecules Comparison.html"):
    """
    Creation of HTML file for 3D visualizing of specific molecules.
    """
    fig = make_subplots(rows=2, cols=2,
                        specs=[[{'type': 'scene'}, {'type': 'scene'}],
                            [{'type': 'scene'}, {'type': 'scene'}]],
                        subplot_titles=tuple(molecules.keys()),
                        horizontal_spacing=0.02,
                        vertical_spacing=0.05
    )
    def draw_sphere(fig, row, col, rad, phi_init=0., phi_end=2*np.pi, theta_init=0, theta_end=np.pi, mesh=100, color='grey', opacity=0.2):
        phi = np.linspace(phi_init, phi_end, mesh)
        theta = np.linspace(theta_init, theta_end, mesh)
        x_vol = rad*np.outer(np.sin(theta), np.cos(phi))
        y_vol = rad*np.outer(np.sin(theta), np.sin(phi))
        z_vol = rad*np.outer(np.cos(theta), np.ones(np.size(phi)))

        fig.add_trace(
            go.Surface(
                x=x_vol, y=y_vol, z=z_vol,
                opacity=0.2,
                colorscale=[[0., color],[1., color]],
                showscale=False,
                hoverinfo='skip'
            ),
            row=row, col=col
        )

    def draw_tube(fig, row, col, r_in, r_out, height, phi_init=0., phi_end=2*np.pi, mesh=50, color='grey', opacity=0.2):
        # 1. Параметры сетки
        phi = np.linspace(0, 2*np.pi, mesh)
        z_range = np.linspace(-height/2, height/2, mesh)
        phi_grid, z_grid = np.meshgrid(phi, z_range)
        
        # --- А. ВНЕШНЯЯ СТЕНКА ---
        x_out = r_out * np.cos(phi_grid)
        y_out = r_out * np.sin(phi_grid)
        fig.add_trace(go.Surface(
            x=x_out, y=y_out, z=z_grid,
            opacity=opacity, showscale=False, colorscale=[[0, color], [1, color]],
            hoverinfo='skip'
        ), row=row, col=col)

        # --- Б. ВНУТРЕННЯЯ СТЕНКА ---
        x_in = r_in * np.cos(phi_grid)
        y_in = r_in * np.sin(phi_grid)
        fig.add_trace(go.Surface(
            x=x_in, y=y_in, z=z_grid,
            opacity=opacity, showscale=False, colorscale=[[0, color], [1, color]],
            hoverinfo='skip'
        ), row=row, col=col)

        # --- В. ТОРЦЫ (ВЕРХНЕЕ И НИЖНЕЕ КОЛЬЦО) ---
        # Для колец нам нужно варьировать РАДИУС от r_in до r_out
        r_range = np.linspace(r_in, r_out, mesh//2)
        # Сетка: угол и радиус
        phi_cap, r_cap = np.meshgrid(phi, r_range)
        
        x_cap = r_cap * np.cos(phi_cap)
        y_cap = r_cap * np.sin(phi_cap)
        
        # Верхняя крышка (z = +height/2)
        fig.add_trace(go.Surface(
            x=x_cap, y=y_cap, z=np.full_like(x_cap, height/2),
            opacity=opacity, showscale=False, colorscale=[[0, color], [1, color]],
            hoverinfo='skip'
        ), row=row, col=col)

        # Нижняя крышка (z = -height/2)
        fig.add_trace(go.Surface(
            x=x_cap, y=y_cap, z=np.full_like(x_cap, -height/2),
            opacity=opacity, showscale=False, colorscale=[[0, color], [1, color]],
            hoverinfo='skip'
        ), 
        row=row, col=col)

    def create_molecule(row, col, name):
        try:
            df = pd.read_csv(molecules[name]["csv"])
            df = df.iloc[::molecules[name]["skip"]]
        except Exception as e:
            print(f"Error at reading CSV file: {e}")
            return
        x = df["x"]; y = df["y"]; z = df["z"]
        colors = df['element'].map(color_map).fillna("black")
        radius = df['element'].map(radius_map).fillna(0.5)*3
        volume = name

        fig.add_trace(
            go.Scatter3d(
                x=x, y=y, z=z,
                mode="markers",
                marker=dict(
                    size=radius,
                    sizemode="diameter",
                    opacity=1.,
                    color=colors,
                    line=dict(width=1, color="black")
                ),
                name=f"Pict.{row}.{col}"
            ),
            row=row, col=col
        )

        
        if (volume == "histone"):
            rad = 25
            draw_sphere(fig, row, col, rad)

        if (volume == "NMDA"):
            h = 180
            rad = 40
            draw_tube(fig, row, col, 0., rad, h)

        if (volume == "ribosome"):
            rad = 125
            draw_sphere(fig, row, col, rad)

        if (volume == "cytoskeleton"):
            h = 180
            rad_min = 75
            rad_max = 125
            draw_tube(fig, row, col, rad_min, rad_max, h)
    
    for i in range(len(molecules.keys())):
        print(i//2+1, i%2+1)
        create_molecule(i//2+1, i%2+1, list(molecules.keys())[i])

    fig.update_scenes(
        xaxis=dict(
            visible=False, # Полностью скрывает ось, сетку и числа
            showgrid=False, 
            zeroline=False, 
            showbackground=False
        ),
        yaxis=dict(visible=False),
        zaxis=dict(visible=False),
        # Улучшаем освещение для "дорогого" вида
        annotations=[] 
    )

    # Убираем внешние поля фигуры
    fig.update_layout(
        title={
            'text': "Macromolecules comparison (Geant4-DNA Validation)",
            'y': 0.98,
            'x': 0.5,
            'xanchor': 'center',
            'yanchor': 'top'
        },
        margin=dict(l=0, r=0, t=0, b=0), # Минимальные отступы
        showlegend=False,
        autosize=True,
        template="plotly_dark",
        paper_bgcolor='black' # Цвет "бумаги" вокруг графиков
    )

    fig.write_html(output_name)
    print(f"File {output_name} was created succsefully!")