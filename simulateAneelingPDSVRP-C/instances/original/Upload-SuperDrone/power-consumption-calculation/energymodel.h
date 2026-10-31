#ifndef ENERGYMODEL_H
#define ENERGYMODEL_H
#define PI 3.14159
#define MAX_ALPHA 45.0
#include <vector>
using namespace std;

class DroneConfig
{
public:
    const int num_drones;
    const double frame_weight;
    const double battery_weight;
    const int num_propellers; // default 4 for quadcopter
    const double r; // (m)
    const double single_capacity; // Ah
    const double voltage;
    const double max_discharge_current; // 40
    vector<double> flightTime;

    DroneConfig(int numDrones,
                double w,
                double bweight,
                int num_propellers,
                double r,
                double e,
                double vol,
                double max_discharge_current
                ) // 20 2.8 kg | 12 1.55 kg
        : num_drones(numDrones),
          frame_weight(w),
          battery_weight(bweight),
          num_propellers(num_propellers),
          r(r),
          single_capacity(e),
          voltage(vol),
          max_discharge_current(max_discharge_current){}
    double getW_single_drone() const{ return frame_weight + battery_weight; }
    double getA() const{ return num_drones * num_propellers *  PI * r * r; }
    double getActualCapacity(const double & load) const{  // depend on load
        return num_drones * single_capacity;
    }
    bool checkCapacity(const double & c) const{
        return c >= 0.1 * single_capacity;
    }
};

class EnergyModel
{
public:
    const double g = 9.81; // (m2/s)
    const double h_flight; // (m)
    const double v_takeoff; // (m/s) vertical velocity during take off
    const double v_landing; // (m/s) vertical velocity during landing
    const double rho; // (kg/m3) density of air
    const double peukert_constant;

    EnergyModel(double hflight, double vtakeoff, double vlanding, double rho, double peukert);

    double get_P_takeoff(const DroneConfig &dc, const double &parcel_weight);
    double get_P_landing(const DroneConfig &dc, const double &parcel_weight);
    double get_P_flight(const DroneConfig &dc, const double &w_parcel, const double &v_h);
    double get_P_hover(const DroneConfig &dc, const double &w_parcel);
    std::vector<double> get_optimal_power(const DroneConfig &dc, const double &w_parcel, const double &distance);

    double get_endurance(const DroneConfig &dc, const double &w_parcel, const double &distance); // based on Peukert effect

    double get_alpha(const DroneConfig &dc, const double &w_parcel, const double &vh);
    double get_k1();
    double get_k2(const DroneConfig &dc);

    double get_c1(const DroneConfig &dc);
    double get_c2();
    double get_c3();
    double get_c4(const DroneConfig &dc);
    double get_c5(const DroneConfig &dc);
    double get_c6();

    void show_discharge_curve(DroneConfig &dc);


};

#endif // ENERGYMODEL_H
