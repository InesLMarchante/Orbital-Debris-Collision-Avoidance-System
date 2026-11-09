import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

# ------------------------------ 1. Load data ------------------------------

df = pd.read_csv('orbital_data.csv')

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
fig.patch.set_facecolor('#EDE8D0')
ax.set_facecolor('#EDE8D0')

# -------------------------- 3. Draw Planet Earth --------------------------

u, v = np.mgrid[0:2*np.pi:30j, 0:np.pi:20j]
x_earth = 6371 * np.cos(u) * np.sin(v)
y_earth = 6371 * np.sin(u) * np.sin(v)
z_earth = 6371 * np.cos(v)
ax.plot_wireframe(x_earth, y_earth, z_earth, color="#0055ff", alpha=0.15)

# ------------------- 4. Extract safe target coordinates -------------------

target = df.iloc[0]
target_x, target_y, target_z = target['X'], target['Y'], target['Z']

# ------------------- 5. Dynamically separate the fleet --------------------

is_deb = df['Name'].str.contains('DEB|FENGYUN|IRIDIUM|COSMOS', case=False, na=False)
active_sats = df[~is_deb & (df.index != target.name)]
debris = df[is_deb & (df.index != target.name)]

if len(debris) == 0:
    active_sats = df.iloc[1:2000]
    debris = df.iloc[2000:]

scatter_ativos = ax.scatter(active_sats['X'], active_sats['Y'], active_sats['Z'], color='#00CC00', s=2, alpha=0.4, label='Active Satellites', zorder=1)
scatter_detritos = ax.scatter(debris['X'], debris['Y'], debris['Z'], color='#ff0000', s=5, alpha=0.9, label='Space Debris', zorder=2)

# ---------- 6. Target and Collision Threat (Static configuration) ---------- 

ax.plot([target_x], [target_y], [target_z], marker='*', color='yellow', markersize=20, markeredgecolor='black', linestyle='None', label='Target', zorder=10)

if os.path.exists('collision_alert.csv'):
    collisions = pd.read_csv('collision_alert.csv')
    collisions['X'] = pd.to_numeric(collisions['X'], errors='coerce')
    collisions = collisions.dropna(subset=['X'])
    
    for index, neighbor in collisions.iterrows():
        ax.plot([target_x, neighbor['X']], [target_y, neighbor['Y']], [target_z, neighbor['Z']], color='magenta', linewidth=2, linestyle='dashed', zorder=5)
        
        if abs(target_x - neighbor['X']) > 0.1:
            ax.plot([neighbor['X']], [neighbor['Y']], [neighbor['Z']], marker='o', color='None', markeredgecolor='magenta', markersize=18, markeredgewidth=2, linestyle='None', zorder=11, label='Imminent Threat!' if index == 1 else "")

# ----------------------- 7. Aesthetics configuration -----------------------

ax.set_title("Collision Avoidance System - Octree 3D\n(Press 'Z': Zoom | '1': Toggle Active | '2': Toggle Debris)", color='black', fontsize=13, fontweight='bold', pad=15)

legend = ax.legend(facecolor="#FFFFFF", edgecolor='gray', labelcolor='black', 
                   loc='upper left', bbox_to_anchor=(-0.15, 0.95), 
                   borderpad=1.2, labelspacing=1.2, handletextpad=1.0)

legend_handles = legend.legend_handles
if len(legend_handles) >= 4:
    legend_handles[0].set_sizes([120])  
    legend_handles[1].set_sizes([120])  
    legend_handles[2].set_markersize(14) 
    legend_handles[3].set_markersize(14) 

ax.tick_params(colors='black')
ax.xaxis.label.set_color('black'); ax.yaxis.label.set_color('black'); ax.zaxis.label.set_color('black')

# ----------------- 8. KEYBOARD SHORTCUTS (Zoom & Filters) -----------------

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

    # ---------- Hide/Show Active Satellites ----------
    
    elif event.key == '1': 
        visible = not scatter_ativos.get_visible()
        scatter_ativos.set_visible(visible)
        fig.canvas.draw()

    # ------------- Hide/Show Space Debris -------------
        
    elif event.key == '2': 
        visible = not scatter_detritos.get_visible()
        scatter_detritos.set_visible(visible)
        fig.canvas.draw()

fig.canvas.mpl_connect('key_press_event', on_key_press)
ax.set_xlim([-15000, 15000]); ax.set_ylim([-15000, 15000]); ax.set_zlim([-15000, 15000])

plt.show()
