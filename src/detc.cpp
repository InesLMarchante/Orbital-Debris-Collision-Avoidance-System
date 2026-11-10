#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

// ------------------------------ 1. Data Structures ------------------------------

struct Satellite {
    string name;
    double inclination;
    double RAAN;
    double eccentricity;
    double arg_perigee; 
    double mean_anomaly;
    double mean_motion;
};

struct Coordinates2D {
    double x;
    double y;
};

struct Coordinates3D {
    double x, y, z;
};

// ------------------------------ 2. Spatial Partitioning Core ------------------------------

struct Box3D {
    Coordinates3D center;
    double box_size; // Half-dimension of the box

    bool isInsideBox (Coordinates3D point) {
        if (point.x >= (center.x - box_size) && point.x <= (center.x + box_size) && 
            point.y >= (center.y - box_size) && point.y <= (center.y + box_size) && 
            point.z >= (center.z - box_size) && point.z <= (center.z + box_size)) {
            return true;
        }
        return false;
    }

    bool intersects(Box3D other) {
        if (std::abs(center.x - other.center.x) > (box_size + other.box_size)) return false;
        if (std::abs(center.y - other.center.y) > (box_size + other.box_size)) return false;
        if (std::abs(center.z - other.center.z) > (box_size + other.box_size)) return false;
        return true; 
    }
};

class Octree {
public:
    Box3D boundary; 
    int capacity = 4; 
    std::vector<Coordinates3D> points;

    bool box_divided = false;
    Octree* children[8];

    Octree(Box3D limit) {
        boundary = limit;
    }

    ~Octree() {
        if (box_divided) {
            for (int i = 0; i < 8; i++) {
                delete children[i]; 
            }
        }
    }

    bool insert(Coordinates3D p) {
        if (!boundary.isInsideBox(p)) return false;
        if (points.size() < capacity) {
            points.push_back(p); 
            return true;
        }
        if (!box_divided) {
            subdivide(); 
            box_divided = true; 
        }
        for (int i = 0; i < 8; i++) {
            if (children[i]->insert(p)) return true; 
        }
        return false;
    }

    void subdivide() {
        double new_size = boundary.box_size / 2.0;
        double x = boundary.center.x;
        double y = boundary.center.y;
        double z = boundary.center.z;

        Coordinates3D c0 = {x - new_size, y - new_size, z - new_size};
        Coordinates3D c1 = {x + new_size, y - new_size, z - new_size};
        Coordinates3D c2 = {x - new_size, y + new_size, z - new_size};
        Coordinates3D c3 = {x + new_size, y + new_size, z - new_size};
        Coordinates3D c4 = {x - new_size, y - new_size, z + new_size};
        Coordinates3D c5 = {x + new_size, y - new_size, z + new_size};
        Coordinates3D c6 = {x - new_size, y + new_size, z + new_size};
        Coordinates3D c7 = {x + new_size, y + new_size, z + new_size};
        
        children[0] = new Octree({c0, new_size}); children[1] = new Octree({c1, new_size});
        children[2] = new Octree({c2, new_size}); children[3] = new Octree({c3, new_size});
        children[4] = new Octree({c4, new_size}); children[5] = new Octree({c5, new_size});
        children[6] = new Octree({c6, new_size}); children[7] = new Octree({c7, new_size});
    }

    void query(Box3D search_area, std::vector<Coordinates3D>& found_objects) {
        if (!boundary.intersects(search_area)) return; 
        for (size_t i = 0; i < points.size(); i++) {
            if (search_area.isInsideBox(points[i])) {
                found_objects.push_back(points[i]);
            }
        }
        if (box_divided) {
            for (int i = 0; i < 8; i++) {
                children[i]->query(search_area, found_objects);
            }
        }
    }
};

// ------------------------------ 3. Orbital Mechanics Math ------------------------------

double keplerEquation(double M_degrees, double e) {
    double M = M_degrees * (M_PI / 180.0); 
    double E = M; 
    double error = 1.0; 
    double tolerance = 0.000001; 

    while (error > tolerance) {
        double E_new = E - ((E - e * sin(E) - M) / (1.0 - e * cos(E)));
        error = std::abs(E_new - E);
        E = E_new;
    }
    return E; 
}

Coordinates2D calculate2DPlane(double mean_motion, double E, double e) {
    double GM = 398600.4418; 
    double n = mean_motion * (2.0 * M_PI) / 86400.0; 
    double a = cbrt(GM / pow(n, 2)); 
    Coordinates2D plane;
    plane.x = a * (cos(E) - e);
    plane.y = a * sqrt(1.0 - pow(e, 2)) * sin(E);
    return plane;
}

Coordinates3D calculate3DPosition(double x_derivative, double y_derivative, double incl_degrees, double RAAN_degrees, double arg_perigee_degrees) {
    double i = incl_degrees * (M_PI / 180.0);
    double RAAN = RAAN_degrees * (M_PI / 180.0);
    double w = arg_perigee_degrees * (M_PI / 180.0);
    Coordinates3D pos;
    pos.x = x_derivative * (cos(RAAN)*cos(w) - sin(RAAN)*sin(w)*cos(i)) + y_derivative * (-cos(RAAN)*sin(w) - sin(RAAN)*cos(w)*cos(i));
    pos.y = x_derivative * (sin(RAAN)*cos(w) + cos(RAAN)*sin(w)*cos(i)) + y_derivative * (-sin(RAAN)*sin(w) + cos(RAAN)*cos(w)*cos(i));
    pos.z = x_derivative * (sin(w)*sin(i)) + y_derivative * (cos(w)*sin(i));
    return pos;
}

// ------------------------------ 4. Main Execution ------------------------------

int main() {
    vector<string> files = {
        "data/active.txt", 
        "data/iridium-33-debris.txt", 
        "data/fengyun-1c-debris.txt"
    };
    
    int total_sat_count = 0;
    vector<Satellite> fleet; 

    for (const string& filename : files) {
        fstream input(filename, ios::in);
        if (!input.is_open()) continue;
        string line;
        while (getline(input, line)) {
            if (line.find("GP data") != string::npos || line.find("Data is updated") != string::npos || line.empty()) continue; 
            
            string sat_name = line;
            string line1, line2;
            getline(input, line1); getline(input, line2);

            if (line2.length() < 25) continue; 
            
            Satellite temp_sat; 
            temp_sat.name           = sat_name; 
            temp_sat.inclination    = stod(line2.substr(8, 8));
            temp_sat.RAAN           = stod(line2.substr(17, 8));
            temp_sat.eccentricity   = stod("0." + line2.substr(26, 7)); 
            temp_sat.arg_perigee    = stod(line2.substr(34, 8));
            temp_sat.mean_anomaly   = stod(line2.substr(43, 8));
            temp_sat.mean_motion    = stod(line2.substr(52, 11));
            fleet.push_back(temp_sat);
            total_sat_count++;
        }
        input.close(); 
    }
  
    cout << "\n=========================================================" << endl;
    cout << "              ORBITAL DEBRIS TRACKING SYSTEM             " << endl;
    cout << "=========================================================" << endl;
    cout << "[+] Database loaded: " << total_sat_count << " objects in memory." << endl;
    
    if (!fleet.empty()) {
        cout << "\n[+] INITIALIZING 3D SPATIAL ENGINE (OCTREE)..." << endl;
        Octree universe({ {0.0, 0.0, 0.0}, 100000.0 });
        ofstream export_file("output/orbital_data.csv");
        export_file << "Name,X,Y,Z\n"; 

        for (size_t i = 0; i < fleet.size(); i++) {
            double E = keplerEquation(fleet[i].mean_anomaly, fleet[i].eccentricity);
            Coordinates2D plane = calculate2DPlane(fleet[i].mean_motion, E, fleet[i].eccentricity);
            Coordinates3D space_pos = calculate3DPosition(plane.x, plane.y, fleet[i].inclination, fleet[i].RAAN, fleet[i].arg_perigee);
            export_file << fleet[i].name << "," << space_pos.x << "," << space_pos.y << "," << space_pos.z << "\n";
            universe.insert(space_pos);
        }
        export_file.close(); 
        cout << "[+] Engine online. Space successfully partitioned." << endl;

        // ------------------------------ 7. Interactive Target Acquisition ------------------------------
        
        cout << "\n=========================================================" << endl;
        cout << "                 LAUNCH SITE SELECTION                   " << endl;
        cout << "=========================================================" << endl;
        cout << " Select launch origin for your payload:" << endl;
        cout << " 1. French Guiana (Kourou)       - Lat: 5.2 deg" << endl;
        cout << " 2. USA (Cape Canaveral)         - Lat: 28.5 deg" << endl;
        cout << " 3. Portugal (Azores Spaceport)  - Lat: 37.0 deg" << endl;
        cout << " 4. Kazakhstan (Baikonur)        - Lat: 45.9 deg" << endl;
        cout << " 5. [DEMO] Stress Test - Force Intercept Debris Field" << endl;
        cout << " > ";
        
        int choice;
        cin >> choice;

        Satellite target_sat;
        target_sat.name = "CUSTOM LAUNCH PAYLOAD";
        target_sat.eccentricity = 0.001; 
        target_sat.arg_perigee = 0.0;
        target_sat.mean_anomaly = 90.0;  
        target_sat.mean_motion = 15.5;   

        double launch_lat = 0.0;
        double launch_lon = 0.0;

        switch(choice) {
            case 1: launch_lat = 5.2; launch_lon = -52.8; break;
            case 2: launch_lat = 28.5; launch_lon = -80.6; break;
            case 3: launch_lat = 37.0; launch_lon = -25.1; break;
            case 4: launch_lat = 45.9; launch_lon = 63.3; break;
            case 5: {
                // HACK DE PORTFÓLIO: Copia a órbita de um detrito real e põe o foguetão quase em cima dele!
                int idx = fleet.size() > 50 ? fleet.size() - 50 : 0; 
                target_sat.eccentricity = fleet[idx].eccentricity;
                target_sat.arg_perigee = fleet[idx].arg_perigee;
                target_sat.mean_motion = fleet[idx].mean_motion;
                target_sat.mean_anomaly = fleet[idx].mean_anomaly + 0.1; // +0.1 graus = a poucos km de distância!
                launch_lat = fleet[idx].inclination;
                launch_lon = fleet[idx].RAAN + 90.0;
                break;
            }
            default: launch_lat = 37.0; launch_lon = -25.1; break;
        }

        target_sat.inclination = launch_lat;
        target_sat.RAAN = launch_lon - 90.0;
        
        // Normalização de ângulos
        while(target_sat.RAAN < 0.0) target_sat.RAAN += 360.0;
        while(target_sat.RAAN >= 360.0) target_sat.RAAN -= 360.0;

        if (choice == 5) {
            cout << "\n[!] INITIATING STRESS TEST: Target injected into known debris field trajectory." << endl;
        } else {
            cout << "\n[+] Launch trajectory confirmed. Injecting into LEO from Lat: " << launch_lat << " Lon: " << launch_lon << endl;
        }

        // ----------------- 8. COLLISION AVOIDANCE SYSTEM -----------------
      
        double E_target = keplerEquation(target_sat.mean_anomaly, target_sat.eccentricity);
        Coordinates2D target_plane = calculate2DPlane(target_sat.mean_motion, E_target, target_sat.eccentricity);
        Coordinates3D target_pos = calculate3DPosition(target_plane.x, target_plane.y, target_sat.inclination, target_sat.RAAN, target_sat.arg_perigee);

        Box3D collision_alert = {target_pos, 50.0}; 
        std::vector<Coordinates3D> dangerous_neighbors;
        universe.query(collision_alert, dangerous_neighbors);

        cout << "\n=========================================================" << endl;
        cout << "               COLLISION AVOIDANCE SYSTEM                " << endl;
        cout << "=========================================================" << endl;
        cout << ">>> Target Coordinates : [ X: " << target_pos.x << " | Y: " << target_pos.y << " | Z: " << target_pos.z << " ]" << endl;
        cout << ">>> Scanning radius    : 50.0 km" << endl;
        cout << "\n[!] ALERT REPORT:" << endl;
        cout << "    Objects detected in zone : " << dangerous_neighbors.size() << "\n" << endl;

        for (size_t j = 0; j < dangerous_neighbors.size(); j++) {
            double distance = sqrt(pow(dangerous_neighbors[j].x - target_pos.x, 2) + 
                                   pow(dangerous_neighbors[j].y - target_pos.y, 2) + 
                                   pow(dangerous_neighbors[j].z - target_pos.z, 2));
            
            cout << "    -> Contact " << (j + 1) << " | Distance to target: " << distance << " km";
            if (distance < 0.1) cout << " (Self - Target Confirmed)";
            cout << endl;
        }
        cout << "=========================================================\n" << endl;

      // ------------------------------ 9. Alert Export ------------------------------

        ofstream alert_file("output/collision_alert.csv");
        alert_file << "Type,X,Y,Z\n"; // Header updated
        alert_file << "TARGET," << target_pos.x << "," << target_pos.y << "," << target_pos.z << "\n";
        for (size_t j = 0; j < dangerous_neighbors.size(); j++) {
            alert_file << "DEBRIS," << dangerous_neighbors[j].x << "," << dangerous_neighbors[j].y << "," << dangerous_neighbors[j].z << "\n";
        }
        alert_file.close();
    }

    return 0;
}
