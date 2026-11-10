import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os
from PIL import Image

# ------------------------------ 1. Load data ------------------------------

df = pd.read_csv('output/orbital_data.csv')

# Force numeric values and drop satellites yielding "Infinity" or "NaN"
df['X'] = pd.to_numeric(df['X'], errors='coerce')
df['Y'] = pd.to_numeric(df['Y'], errors='coerce')
df['Z'] = pd.to_numeric(df['Z'], errors='coerce')
df = df.replace([np.inf, -np.inf], np.nan).dropna(subset=['X', 'Y', 'Z'])

if len(df) == 0:
    print("ERROR: C++ engine did not export valid data. Check TLE files!")
    exit()

# ---------------------- 2. Configure 3D environment -----------------------

fig = plt.figure(figsize=(13, 9))
ax = fig.add_subplot(111, projection='3d')

# DARK MODE: Space aesthetic
space_bg = '#08080c'
fig.patch.set_facecolor(space_bg)
ax.set_facecolor(space_bg)

# Make 3D panes transparent and grids subtle
ax.xaxis.set_pane_color((1.0, 1.0, 1.0, 0.0))
ax.yaxis.set_pane_color((1.0, 1.0, 1.0, 0.0))
ax.zaxis.set_pane_color((1.0, 1.0, 1.0, 0.0))
ax.xaxis._axinfo["grid"]['color'] = (1, 1, 1, 0.1)
ax.yaxis._axinfo["grid"]['color'] = (1, 1, 1, 0.1)
ax.zaxis._axinfo["grid"]['color'] = (1, 1, 1, 0.1)

# -------------------------- 3. Draw Planet Earth (Textured) --------------------------

# Create spherical grid
lons = np.linspace(-180, 180, 100) * np.pi/180
lats = np.linspace(-90, 90, 50)[::-1] * np.pi/180
lons, lats = np.meshgrid(lons, lats)

x_earth = 6371 * np.cos(lons) * np.cos(lats)
y_earth = 6371 * np.sin(lons) * np.cos(lats)
z_earth = 6371 * np.sin(lats)

try:
    # Load and resize the texture to match the grid resolution (100x50)
    img = Image.open('data/2k_earth_daymap.jpg')
    img = img.resize((100, 50))
    img_color = np.array(img) / 255.0
    
    # Map the texture to the surface
    ax.plot_surface(x_earth, y_earth, z_earth, facecolors=img_color, rstride=1, cstride=1, antialiased=True, zorder=0)
except FileNotFoundError:
    print("Texture 'data/earth.jpg' not found. Falling back to solid blue sphere.")
    ax.plot_surface(x_earth, y_earth, z_earth, color="#0b5394", alpha=0.5, edgecolor="#00ffff", linewidth=0.2, zorder=0)

# ------------------- 4. Extract Target and Alerts -------------------

target_x, target_y, target_z = 0, 0, 0 # Fallback

if os.path.exists('output/collision_alert.csv'):
    collisions = pd.read_csv('output/collision_alert.csv')
    
    # Retrieve the custom target position injected by C++
    target_data = collisions[collisions['Type'] == 'TARGET']
    if not target_data.empty:
        target_x = target_data.iloc[0]['X']
        target_y = target_data.iloc[0]['Y']
        target_z = target_data.iloc[0]['Z']
        
    ax.plot([target_x], [target_y], [target_z], marker='*', color='gold', markersize=18, markeredgecolor='white', markeredgewidth=1, linestyle='None', label='Launch Payload', zorder=10)

    # Plot dangerous debris vectors
    debris_alerts = collisions[collisions['Type'] == 'DEBRIS']
    for index, neighbor in debris_alerts.iterrows():
        ax.plot([target_x, neighbor['X']], [target_y, neighbor['Y']], [target_z, neighbor['Z']], color='#ff00ff', linewidth=2.5, linestyle='dashed', alpha=0.9, zorder=5)
        ax.plot([neighbor['X']], [neighbor['Y']], [neighbor['Z']], marker='o', color='None', markeredgecolor='#ff00ff', markersize=16, markeredgewidth=2.5, linestyle='None', zorder=11, label='Imminent Threat!' if index == 1 else "")

# -------------------- 5. Dynamically separate the fleet --------------------

is_deb = df['Name'].str.contains('DEB|FENGYUN|IRIDIUM|COSMOS', case=False, na=False)
active_sats = df[~is_deb]
debris = df[is_deb]

if len(debris) == 0:
    active_sats = df.iloc[1:2000]
    debris = df.iloc[2000:]

# High-contrast colors for dark background
scatter_ativos = ax.scatter(active_sats['X'], active_sats['Y'], active_sats['Z'], color='#00ffcc', s=1.5, alpha=0.5, label='Active Satellites', zorder=1)
scatter_detritos = ax.scatter(debris['X'], debris['Y'], debris['Z'], color='#ff3333', s=4, alpha=0.8, label='Space Debris', zorder=2)

# ---------------------- 7. Aesthetics configuration ----------------------

ax.set_title("Collision Avoidance System - Octree 3D\n(Press 'Z': Zoom | '1': Toggle Active | '2': Toggle Debris)", color='white', fontsize=14, fontweight='bold', pad=15)

legend = ax.legend(facecolor="#111111", edgecolor='#444444', labelcolor='white', 
                   loc='upper left', bbox_to_anchor=(-0.15, 0.95), 
                   borderpad=1.2, labelspacing=1.2, handletextpad=1.0)

legend_handles = legend.legend_handles
for handle in legend_handles:
    # Se for um scatter plot (Satélites e Detritos)
    if hasattr(handle, 'set_sizes'):
        handle.set_sizes([120])
    # Se for um plot normal (Alvo e Alertas)
    elif hasattr(handle, 'set_markersize'):
        handle.set_markersize(14)

ax.tick_params(colors='lightgray', labelsize=9)
ax.xaxis.label.set_color('lightgray'); ax.yaxis.label.set_color('lightgray'); ax.zaxis.label.set_color('lightgray')

# ---------------- 8. INTERACTIVITY: KEYBOARD SHORTCUTS ----------------

zoomed = False
def on_key_press(event):
    global zoomed, scatter_ativos, scatter_detritos
    
    if event.key.lower() == 'z':
        if zoomed:
            ax.set_xlim([-15000, 15000])
            ax.set_ylim([-15000, 15000])
            ax.set_zlim([-15000, 15000])
            zoomed = False
        else:
            radius = 500
            ax.set_xlim([target_x - radius, target_x + radius])
            ax.set_ylim([target_y - radius, target_y + radius])
            ax.set_zlim([target_z - radius, target_z + radius])
            zoomed = True
        fig.canvas.draw()
        
    elif event.key == '1': 
        visible = not scatter_ativos.get_visible()
        scatter_ativos.set_visible(visible)
        fig.canvas.draw()

    elif event.key == '2': 
        visible = not scatter_detritos.get_visible()
        scatter_detritos.set_visible(visible)
        fig.canvas.draw()

fig.canvas.mpl_connect('key_press_event', on_key_press)
ax.set_xlim([-15000, 15000]); ax.set_ylim([-15000, 15000]); ax.set_zlim([-15000, 15000])

plt.show()
