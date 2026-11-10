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

    // ------------- 2.1. Checks if a point is inside this 3D box boundary -------------

    bool isInsideBox (Coordinates3D point) {
        if (point.x >= (center.x - box_size) && point.x <= (center.x + box_size) && 
            point.y >= (center.y - box_size) && point.y <= (center.y + box_size) && 
            point.z >= (center.z - box_size) && point.z <= (center.z + box_size)) {
            return true;
        }
        return false;
    }

    // ------------- 2.2. Checks if two boxes overlap (AABB Collision Detection) -------------

    bool intersects(Box3D other) {
        // If the distance between centers is greater than the sum of their half-sizes, they don't touch
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

    // ------------------- 2.3. Constructor -------------------

    Octree(Box3D limit) {
        boundary = limit;
    }

    // -------- 2.4. Destructor to prevent memory leaks --------

    ~Octree() {
        if (box_divided) {
            for (int i = 0; i < 8; i++) {
                delete children[i]; 
            }
        }
    }

    bool insert(Coordinates3D p) {
        // ---------- 2.5. - Phase 1: Boundary check ----------
        
        if (!boundary.isInsideBox(p)) {
            return false;
        }

        // ---------- 2.6. - Phase 2: Capacity check ----------
        
        if (points.size() < capacity) {
            points.push_back(p); 
            return true;
        }

        // ----------- 2.7. - Phase 3: Subdivision ------------
        
        if (!box_divided) {
            subdivide(); 
            box_divided = true; 
        }

        for (int i = 0; i < 8; i++) {
            if (children[i]->insert(p)) {
                return true; 
            }
        }
        return false;
    }

    void subdivide() {
        double new_size = boundary.box_size / 2.0;
        double x = boundary.center.x;
        double y = boundary.center.y;
        double z = boundary.center.z;

        // ----------- 2.8. Calculate the 8 new geometric centers -----------

        Coordinates3D c0 = {x - new_size, y - new_size, z - new_size};
        Coordinates3D c1 = {x + new_size, y - new_size, z - new_size};
        Coordinates3D c2 = {x - new_size, y + new_size, z - new_size};
        Coordinates3D c3 = {x + new_size, y + new_size, z - new_size};
        Coordinates3D c4 = {x - new_size, y - new_size, z + new_size};
        Coordinates3D c5 = {x + new_size, y - new_size, z + new_size};
        Coordinates3D c6 = {x - new_size, y + new_size, z + new_size};
        Coordinates3D c7 = {x + new_size, y + new_size, z + new_size};

        // ---------------- 2.9. Create the 8 physical boxes ----------------
        
        Box3D b0 = {c0, new_size}; Box3D b1 = {c1, new_size};
        Box3D b2 = {c2, new_size}; Box3D b3 = {c3, new_size};
        Box3D b4 = {c4, new_size}; Box3D b5 = {c5, new_size};
        Box3D b6 = {c6, new_size}; Box3D b7 = {c7, new_size};

        // -------- 2.10. Allocate the 8 new child Octrees in memory ---------
        
        children[0] = new Octree(b0); children[1] = new Octree(b1);
        children[2] = new Octree(b2); children[3] = new Octree(b3);
        children[4] = new Octree(b4); children[5] = new Octree(b5);
        children[6] = new Octree(b6); children[7] = new Octree(b7);
    }

    void query(Box3D search_area, std::vector<Coordinates3D>& found_objects) {
        // ----- 2.11. - Phase 1:  check (Performance optimization) -----
        
        if (!boundary.intersects(search_area)) {
            return; 
        }

        // ----------------- 2.12. - Phase 2: Check local points -----------------
        
        for (size_t i = 0; i < points.size(); i++) {
            if (search_area.isInsideBox(points[i])) {
                found_objects.push_back(points[i]);
            }
        }

        // ------------ 2.13. - Phase 3: Recursive query to children -------------
        
        if (box_divided) {
            for (int i = 0; i < 8; i++) {
                children[i]->query(search_area, found_objects);
            }
        }
    }
};

// ------------------------------ 3. Orbital Mechanics Math ------------------------------

double keplerEquation(double M_degrees, double e) {
    // Convert Mean Anomaly from degrees to radians
    double M = M_degrees * (M_PI / 180.0); 
    
    double E = M; // Initial guess in radians
    double error = 1.0; 
    double tolerance = 0.000001; 

    // Newton-Raphson method to solve Kepler's Equation
    while (error > tolerance) {
        double E_new = E - ((E - e * sin(E) - M) / (1.0 - e * cos(E)));
        error = std::abs(E_new - E);
        E = E_new;
    }

    return E; // Final Eccentric Anomaly in radians
}

Coordinates2D calculate2DPlane(double mean_motion, double E, double e) {
    double GM = 398600.4418; // Earth's gravitational constant (km^3/s^2)
    
    // Convert mean motion from revs/day to rads/sec
    double n = mean_motion * (2.0 * M_PI) / 86400.0; 
    
    // Kepler's Third Law to find the semi-major axis (a)
    double a = cbrt(GM / pow(n, 2)); 

    // Calculate planar coordinates
    Coordinates2D plane;
    plane.x = a * (cos(E) - e);
    plane.y = a * sqrt(1.0 - pow(e, 2)) * sin(E);

    return plane;
}

Coordinates3D calculate3DPosition(double x_derivative, double y_derivative, double incl_degrees, double RAAN_degrees, double arg_perigee_degrees) {
    // Convert angles to radians for the rotation matrix
    double i = incl_degrees * (M_PI / 180.0);
    double RAAN = RAAN_degrees * (M_PI / 180.0);
    double w = arg_perigee_degrees * (M_PI / 180.0);

    Coordinates3D pos;

    // 3D Rotation Matrix to convert orbital plane to Earth-Centered Inertial (ECI) coordinates
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

  // ------------------------------ 5. File Parsing and Data Ingestion ------------------------------

    for (const string& filename : files) {
        fstream input(filename, ios::in);
        
        if (!input.is_open()) {
            cout << "Warning: File " << filename << " not found." << endl;
            continue;
        }

        string line;
        int file_sat_count = 0;

        while (getline(input, line)) {
            // Skip headers and empty lines
            if (line.find("GP data") != string::npos || line.find("Data is updated") != string::npos || line.empty()) {
                continue; 
            }
            
            string sat_name = line;
            string line1, line2;
            getline(input, line1);
            getline(input, line2);

            // ------------------ 5.1. - Ignore incomplete or corrupted TLE lines ------------------
          
            if (line2.length() < 25) {
                continue; 
            }

            // ---------- 5.2. - Extraction (Specific substring cuts based on TLE format) ----------
          
            string text_inclination = line2.substr(8, 8);
            string text_RAAN        = line2.substr(17, 8);
            string text_eccentricity = line2.substr(26, 7); 
            string text_arg_perigee = line2.substr(34, 8);
            string text_anomaly     = line2.substr(43, 8);
            string text_mean_motion = line2.substr(52, 11);

            // ----------------------- 5.3. - Conversions (String -> Double) -----------------------
          
            double val_inclination = stod(text_inclination);
            double val_RAAN        = stod(text_RAAN);
            
            // ------------- 5.4. - TLE TRICK: Prepend "0." for proper decimal parsing -------------
          
            double val_eccentricity  = stod("0." + text_eccentricity); 
            
            double val_arg_perigee = stod(text_arg_perigee);
            double val_anomaly     = stod(text_anomaly);
            double val_mean_motion = stod(text_mean_motion);

            file_sat_count++;
            total_sat_count++;

            Satellite temp_sat; 
            temp_sat.name           = sat_name; 
            temp_sat.inclination    = val_inclination;
            temp_sat.RAAN           = val_RAAN;
            temp_sat.eccentricity   = val_eccentricity;
            temp_sat.arg_perigee    = val_arg_perigee;
            temp_sat.mean_anomaly   = val_anomaly;
            temp_sat.mean_motion    = val_mean_motion;

            fleet.push_back(temp_sat);
        }
        
        cout << "Success: Read " << file_sat_count << " objects from " << filename << endl;
        input.close(); 
    }

// ------------------------------ 6. Spatial Engine Initialization ------------------------------
  
    cout << "\n=========================================================" << endl;
    cout << "              ORBITAL DEBRIS TRACKING SYSTEM             " << endl;
    cout << "=========================================================" << endl;
    cout << "[+] Database loaded: " << total_sat_count << " objects in memory." << endl;
    
    if (!fleet.empty()) {
      
        // --------------- 6.1. SPATIAL ENGINE INITIALIZATION ----------------
      
        cout << "\n[+] INITIALIZING 3D SPATIAL ENGINE (OCTREE)..." << endl;
        
        Coordinates3D earth_center = {0.0, 0.0, 0.0};
        Box3D space_boundary = {earth_center, 100000.0}; 
        Octree universe(space_boundary);

        int successfully_inserted = 0;

        ofstream export_file("output/orbital_data.csv");
        export_file << "Name,X,Y,Z\n"; 

        for (size_t i = 0; i < fleet.size(); i++) {
            double E = keplerEquation(fleet[i].mean_anomaly, fleet[i].eccentricity);
            Coordinates2D plane = calculate2DPlane(fleet[i].mean_motion, E, fleet[i].eccentricity);
            Coordinates3D space_pos = calculate3DPosition(plane.x, plane.y, fleet[i].inclination, fleet[i].RAAN, fleet[i].arg_perigee);
            
            export_file << fleet[i].name << "," << space_pos.x << "," << space_pos.y << "," << space_pos.z << "\n";

            if (universe.insert(space_pos)) {
                successfully_inserted++;
            }
        }
        
        export_file.close(); 

        cout << "[+] Engine online. Partitioning space limit: 100,000 km" << endl;
        cout << "[+] Success: " << successfully_inserted << " objects mapped to spatial grid." << endl;

        // ------------------------------ 7. Interactive Target Acquisition ------------------------------
        
        cout << "\n=========================================================" << endl;
        cout << "                 LAUNCH SITE SELECTION                   " << endl;
        cout << "=========================================================" << endl;
        cout << " Select launch origin for your payload:" << endl;
        cout << " 1. French Guiana (Kourou)       - Lat: 5.2 deg" << endl;
        cout << " 2. USA (Cape Canaveral)         - Lat: 28.5 deg" << endl;
        cout << " 3. Portugal (Azores Spaceport)  - Lat: 37.0 deg" << endl;
        cout << " 4. Kazakhstan (Baikonur)        - Lat: 45.9 deg" << endl;
        cout << " > ";
        
        int choice;
        cin >> choice;

        Satellite target_sat;
        target_sat.name = "CUSTOM LAUNCH PAYLOAD";
        target_sat.eccentricity = 0.001; // Circular LEO
        target_sat.arg_perigee = 0.0;
        target_sat.mean_anomaly = 90.0;  // Coloca o satélite na latitude máxima da sua órbita
        target_sat.mean_motion = 15.5;   // Approx 400km altitude

        double launch_lat = 0.0;
        double launch_lon = 0.0;

        switch(choice) {
            case 1: // French Guiana (Kourou)
                launch_lat = 5.2;
                launch_lon = -52.8;
                break;
            case 2: // USA (Cape Canaveral)
                launch_lat = 28.5;
                launch_lon = -80.6;
                break;
            case 3: // Portugal (Azores Spaceport, Santa Maria)
                launch_lat = 37.0;
                launch_lon = -25.1;
                break;
            case 4: // Kazakhstan (Baikonur)
                launch_lat = 45.9;
                launch_lon = 63.3;
                break;
            default: // Default to Portugal
                launch_lat = 37.0;
                launch_lon = -25.1;
                break;
        }

        // Aplicação física da posição:
        target_sat.inclination = launch_lat;
        
        // Alinhamento com o mapa 3D da Terra (Longitude - 90 graus devido à rotação ECI)
        target_sat.RAAN = launch_lon - 90.0;
        if (target_sat.RAAN < 0) {
            target_sat.RAAN += 360.0;
        }

        cout << "\n[+] Launch trajectory confirmed. Injecting into LEO from Lat: " << launch_lat << " Lon: " << launch_lon << endl;

        // ----------------- 8. COLLISION AVOIDANCE SYSTEM -----------------
      
        double E_target = keplerEquation(target_sat.mean_anomaly, target_sat.eccentricity);
        Coordinates2D target_plane = calculate2DPlane(target_sat.mean_motion, E_target, target_sat.eccentricity);
        Coordinates3D target_pos = calculate3DPosition(target_plane.x, target_plane.y, target_sat.inclination, target_sat.RAAN, target_sat.arg_perigee);

        // Voltar ao raio de deteção realista de 50 km
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

        // ----- Calculate and display exact distance to each found object -----
      
        for (size_t j = 0; j < dangerous_neighbors.size(); j++) {
            double distance = sqrt(pow(dangerous_neighbors[j].x - target_pos.x, 2) + 
                                   pow(dangerous_neighbors[j].y - target_pos.y, 2) + 
                                   pow(dangerous_neighbors[j].z - target_pos.z, 2));
            
            cout << "    -> Contact " << (j + 1) << " | Distance to target: " << distance << " km";
            if (distance < 0.1) {
                cout << " (Self - Target Confirmed)";
            }
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
