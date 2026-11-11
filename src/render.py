import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os
from PIL import Image
import urllib.request

# ------------------------------ 1. Load data ------------------------------

df = pd.read_csv('output/orbital_data.csv')

# Defense mechanism against C++ math errors
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

lons = np.linspace(-180, 180, 100) * np.pi/180
lats = np.linspace(-90, 90, 50)[::-1] * np.pi/180
lons, lats = np.meshgrid(lons, lats)

x_earth = 6371 * np.cos(lons) * np.cos(lats)
y_earth = 6371 * np.sin(lons) * np.cos(lats)
z_earth = 6371 * np.sin(lats)

try:
    texture_path = 'data/2k_earth_daymap.jpg'
    if not os.path.exists(texture_path):
        print("Texture not found. Auto-downloading Earth map...")
        os.makedirs('data', exist_ok=True)
        urllib.request.urlretrieve("https://upload.wikimedia.org/wikipedia/commons/thumb/c/cd/Land_ocean_ice_2048.jpg/1024px-Land_ocean_ice_2048.jpg", texture_path)
        
    img = Image.open(texture_path)
    img = img.resize((100, 50))
    img_color = np.array(img) / 255.0
    ax.plot_surface(x_earth, y_earth, z_earth, facecolors=img_color, rstride=1, cstride=1, antialiased=True, zorder=0)
except Exception as e:
    print(f"Texture error: {e}. Falling back to solid blue sphere.")
    ax.plot_surface(x_earth, y_earth, z_earth, color="#0b5394", alpha=0.5, edgecolor="#00ffff", linewidth=0.2, zorder=0)

# ------------------- 4. Extract Target and Alerts -------------------

target_x, target_y, target_z = 0, 0, 0 
if os.path.exists('output/collision_alert.csv'):
    collisions = pd.read_csv('output/collision_alert.csv')
    collisions['X'] = pd.to_numeric(collisions['X'], errors='coerce')
    collisions = collisions.dropna(subset=['X'])
    
    target_data = collisions[collisions['Type'] == 'TARGET']
    if not target_data.empty:
        target_x = target_data.iloc[0]['X']
        target_y = target_data.iloc[0]['Y']
        target_z = target_data.iloc[0]['Z']
        
    ax.plot([target_x], [target_y], [target_z], marker='*', color='gold', markersize=18, markeredgecolor='white', markeredgewidth=1, linestyle='None', label='Launch Payload', zorder=10)

    debris_alerts = collisions[collisions['Type'] == 'DEBRIS']
    for index, neighbor in debris_alerts.iterrows():
        ax.plot([target_x, neighbor['X']], [target_y, neighbor['Y']], [target_z, neighbor['Z']], color='#ff00ff', linewidth=2.5, linestyle='dashed', alpha=0.9, zorder=5)
        ax.plot([neighbor['X']], [neighbor['Y']], [neighbor['Z']], marker='o', color='None', markeredgecolor='#ff00ff', markersize=16, markeredgewidth=2.5, linestyle='None', zorder=11)

# -------------------- 5. Dynamically separate the fleet --------------------

is_deb = df['Name'].astype(str).str.contains('DEB', case=False, na=False)

active_sats = df[~is_deb]
debris = df[is_deb]

# Fallback se a regra falhar (divide a meio para pelo menos garantir 2 cores visíveis)
if len(debris) == 0 and len(df) > 1000:
    active_sats = df.iloc[:1000]
    debris = df.iloc[1000:]

scatter_ativos = ax.scatter(active_sats['X'], active_sats['Y'], active_sats['Z'], color='#00ffcc', s=1.5, alpha=0.5, zorder=1)
scatter_detritos = ax.scatter(debris['X'], debris['Y'], debris['Z'], color='#ff3333', s=4, alpha=0.8, zorder=2)

# ---------------------- 7. Aesthetics configuration ----------------------

ax.set_title("Collision Avoidance System - Octree 3D\n(Press 'Z': Zoom | '1': Toggle Active | '2': Toggle Debris)", color='white', fontsize=14, fontweight='bold', pad=15)

from matplotlib.lines import Line2D
custom_legend = [
    Line2D([0], [0], marker='*', color='gold', markersize=14, linestyle='None', label='Launch Payload'),
    Line2D([0], [0], marker='o', color='None', markeredgecolor='#ff00ff', markersize=12, markeredgewidth=2.5, linestyle='None', label='Imminent Threat!'),
    Line2D([0], [0], marker='o', color='#00ffcc', markersize=10, alpha=0.8, linestyle='None', label='Active Satellites'),
    Line2D([0], [0], marker='o', color='#ff3333', markersize=10, alpha=0.9, linestyle='None', label='Space Debris')
]

ax.legend(handles=custom_legend, facecolor="#111111", edgecolor='#444444', labelcolor='white', 
          loc='upper left', bbox_to_anchor=(-0.15, 0.95), 
          borderpad=1.2, labelspacing=1.2, handletextpad=1.0)

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
