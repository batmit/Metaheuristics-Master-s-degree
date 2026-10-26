#include <math.h>
#include <vector>
#include "energymodel.h"
#include "gnuplot-iostream.h"
using namespace std;

double EnergyModel::get_P_takeoff(const DroneConfig &dc, const double &parcel_weight)
{
    const double T = g * (dc.getW_single_drone() + parcel_weight / dc.num_drones);
    return get_k1() *
            T *
            ( v_takeoff/2 +
            sqrt( pow(v_takeoff/2,2) + T/pow( get_k2(dc) , 2 ) ) ) // induced power for lifting
            +
            get_c2() * pow(T, 1.5)          ; // profile power for takeoff
}

double EnergyModel::get_P_landing(const DroneConfig &dc, const double &parcel_weight)
{
    const double T = g * (dc.getW_single_drone() + parcel_weight / dc.num_drones);
    return get_k1() *
            T *
            ( v_landing/2 +
            sqrt( pow(v_landing/2,2) + T/pow( get_k2(dc) , 2 ) ) ) // induced power for lifting
            +
            get_c2() * pow(T, 1.5)          ; // profile power for takeoff
}

double EnergyModel::get_P_flight(const DroneConfig &dc, const double &w_parcel, const double &vh)
{
    const double T = g * (dc.getW_single_drone() + w_parcel / dc.num_drones);
    double alpha = get_alpha(dc, w_parcel, vh);
    double cos_a = cos(alpha * PI / 180);

    return (get_c1(dc) + get_c2()) * pow (   pow(T - get_c5(dc)*pow(vh*cos_a,2) , 2)
                                             +
                                             pow( get_c4(dc)*vh*vh , 2 )
                                     , 0.75)
            +
           get_c4(dc) * pow(vh, 3) ;
}

double EnergyModel::get_P_hover(const DroneConfig &dc, const double &w_parcel)
{
    return (get_c1(dc) + get_c2()) * pow( (dc.getW_single_drone() + w_parcel / dc.num_drones)*g , 1.5 );
}

std::vector<double> EnergyModel::get_optimal_power(const DroneConfig &dc, const double &w_parcel, const double &distance) // distance in m
{
    double vin_best = -1, vout_best = -1;
    double c_used_best = -1;
    double speed_0 = 4; // m/s
    double speed_1 = 28; // m/s
    double i_nominal = dc.single_capacity * 1; // at the beginning, I set this to 1

    double t_ver_takeoff = h_flight / v_takeoff ; // second
    double t_ver_landing = h_flight / v_landing ; // second
    double p_takeoff_in = get_P_takeoff(dc, w_parcel);
    double p_landing_in = get_P_landing(dc, w_parcel);
    double p_takeoff_out = get_P_takeoff(dc, 0); // return with no load
    double p_landing_out = get_P_landing(dc, 0); // return with no load

    double i_takeoff_in = p_takeoff_in / dc.voltage;
    double i_landing_in = p_landing_in / dc.voltage;
    double i_takeoff_out = p_takeoff_out / dc.voltage;
    double i_landing_out = p_landing_out / dc.voltage;

    double i_takeoff_in_eff = i_takeoff_in * pow(i_takeoff_in/i_nominal, peukert_constant-1);
    double i_landing_in_eff = i_landing_in * pow(i_landing_in/i_nominal, peukert_constant-1);
    double i_takeoff_out_eff = i_takeoff_out * pow(i_takeoff_out/i_nominal, peukert_constant-1);
    double i_landing_out_eff = i_landing_out * pow(i_landing_out/i_nominal, peukert_constant-1);

    double c_left = dc.single_capacity
            - (i_takeoff_in_eff + i_takeoff_out_eff) * t_ver_takeoff / 3600 // second to h
            - (i_landing_in_eff + i_landing_out_eff) * t_ver_landing / 3600 ; // second to h
    if (! dc.checkCapacity(c_left) || i_takeoff_in > dc.max_discharge_current) // return null immediately, can't even lift 
        return vector<double>{-1, -1, -1, -1};

    for (double vin = speed_0; vin <= speed_1; vin += 0.2){
        double t_in = distance / vin ; // second
        double p_in = get_P_flight(dc, w_parcel, vin);
        double i_in = p_in / dc.voltage;
        double i_in_eff = i_in * pow(i_in/i_nominal, peukert_constant-1);
        double c_afterIn = c_left - i_in_eff * t_in/3600;

        if (! dc.checkCapacity(c_afterIn) || i_in > dc.max_discharge_current)
            continue;
        for (double vout = speed_1; vout >= speed_0; vout -= 0.2){
            double t_out = distance  / vout ; // second
            double p_out = get_P_flight(dc, 0, vout); // no load
            double i_out = p_out / dc.voltage;
            double i_out_eff = i_out * pow(i_out/i_nominal, peukert_constant-1);
            double c_afterOut = c_afterIn - i_out_eff * t_out / 3600;

            if (! dc.checkCapacity(c_afterOut) || i_out > dc.max_discharge_current)
                continue; // instead of continue
            // update vin pin vout pout
            bool faster = ((1/vin_best + 1/vout_best) >
                           (1/vin + 1/vout));
            bool friendly = (1/vin_best + 1/vout_best == 1/vin + 1/vout)
                    && (dc.single_capacity - c_afterOut < c_used_best);
            if (faster || vin_best == -1 || friendly ){
                vin_best = vin;
                vout_best = vout;
                c_used_best = dc.single_capacity - c_afterOut;
            }
        }
    }
    return vector<double>{vin_best, vout_best, c_used_best};
}

//double EnergyModel::get_endurance(const DroneConfig &dc, const double &w_parcel, const double &distance)
//{

//}

double EnergyModel::get_alpha(const DroneConfig &dc, const double &w_parcel, const double &vh)
{
    for (double alpha = 0; alpha < 90; alpha += 0.5){
        double rad = alpha * PI / 180;
        double lhs = tan(rad) * ((dc.getW_single_drone() + w_parcel / dc.num_drones)*g - get_c5(dc) * pow(vh * cos(rad),2));
        double rhs = get_c4(dc) * vh * vh;
        double delta = abs(lhs - rhs);
        if (delta  <= 5)
            return alpha;
    }

    assert(false); // never get here
    for (double alpha = 0; alpha < 90; alpha += 0.1){
        double rad = alpha * PI / 180;
        double lhs = tan(rad) * ((dc.getW_single_drone() + w_parcel / dc.num_drones)*g - get_c5(dc) * pow(vh * cos(rad),2));
        double rhs = get_c4(dc) * vh * vh;
        double delta = abs(lhs - rhs);
        if (delta  <= 0.2)
            return alpha;
    }

    return -1;
}

double EnergyModel::get_k1()
{
    return 0.8554;
}

double EnergyModel::get_k2(const DroneConfig &dc)
{
    return sqrt ( 2 * rho *  dc.getA());
}



double EnergyModel::get_c1(const DroneConfig &dc) // k1/k2
{
    return get_k1() / get_k2(dc);
}

double EnergyModel::get_c2() // k3^1.5 N c cd rho R4/8    ->>> 0.3177
/* N  the total number of blades in a single propeller
 * cd the drag coefficient of the blade
 * c the blade chord width
*/
{
    return 0.3177;
}

double EnergyModel::get_c3() // contribute very little -> assume c3 = 0 to simplify (Liu, 2017)
{
    return 0;
}

double EnergyModel::get_c4(const DroneConfig &dc) // <<<<<<<<<<<<<<<<<<<<HERE
/*
 * Cd rho Aquad / 2    (kg/m)
 * Cd - drag coefficient of the vehicle body
 * Aquad is the cross-sectional area of the vehicle when
against wind
 * NOTICE: Cd and Aquad are complex function of the vehicle geometry and flight direction, making c4 is hard to identify.
 */
{
    return 0.0296 ;  //     just need consider single drone with shared mass
}

double EnergyModel::get_c5(const DroneConfig &dc)
/*
 * N c cl rho R/4
 * cl lift coefficient
 */
{
    return 0.0279 / 0.15 * dc.r; // assume only true for R = 0.15m
}

double EnergyModel::get_c6() // assume c6 = 0 to simplify (Liu, 2017)
{
    return 0;
}

void EnergyModel::show_discharge_curve(DroneConfig &dc)
{
    Gnuplot gp;

    // prepare data drones deliveries
    vector<double> weights = {1, 2}; // 0, 0.5, , 2, 3

    double speed_0 = 0; // m/s
    double speed_1 = 30; // m/s 12

    gp << "set key outside\n"; // turn off legend altogether
    gp << "set key box lt -1 lw 1\n";

    gp << "plot ";
    vector<vector<double>> vv;
    vector<vector<double>> ee;
    for (double w : weights){
        vector<double> v;
        vector<double> e;

        for (double speed = speed_0; speed <= speed_1; speed += 0.5){
//            cout << w << " " << speed << " m/s - " << get_alpha(dc, w, speed) << " degree" << endl;
            double energy = get_P_flight(dc, w, speed);
            v.push_back(speed);
            e.push_back(energy);
        }
        vv.push_back(v);
        ee.push_back(e);
        gp << "'-' with linespoints,";
    }
    gp << endl;

    for (int i = 0; i < weights.size(); ++i){
        gp.send1d(boost::make_tuple(vv[i], ee[i]));
    }




}

EnergyModel::EnergyModel(double hflight, double vtakeoff, double vlanding, double rho, double peukert): h_flight(hflight), v_takeoff(vtakeoff), v_landing(vlanding), rho(rho), peukert_constant(peukert)
{

}
