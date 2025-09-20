#include "gps_speed.h"

static inline double deg2rad(double d) {return d* M_PI / 180.0;}

double haversine_m(double lat1_deg, double lon1_deg,
                    double lat2_deg, double lon2_deg){
    constexpr double R = 6371000.0; // meters
    const double lat1 = deg2rad(lat1_deg), lon1 = deg2rad(lon1_deg);
    const double lat2 = deg2rad(lat2_deg), lon2 = deg2rad(lon2_deg);
    const double dlat = lat2 - lat1;
    const double dlon= lon2 - lon1;
    const double a = std::sin(dlat * 0.5) * std::sin(dlat * 0.5) +
                     std::cos(lat1) * std::cos(lat2) * std::sin(dlon * 0.5) * std::sin(dlon * .5);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

SpeedEMA::SpeedEMA(double alpha) : alpha_(alpha), inited_(false), ema_(0.0) {}
void SpeedEMA::reset() { inited_ = false; ema_ = 0.0;}
double SpeedEMA::update(double raw) {
    if(!inited_) { ema_ = raw; inited_ = true; return ema_; }
    ema_ = alpha_ * raw + (1.0 - alpha_) * ema_;
    return ema_;
}

SpeedCalc::SpeedCalc(double alpha)
: prev_{0,0,0,false,0}
, filter_(alpha)
, last_mph_(NAN)
, accel_limit_mps2_(12.0)
, max_speed_mps_(85.0)
, dt_min_(0.05)
, dt_max_(2.0)
, hdop_max_(3.0) {}

void SpeedCalc::reset() {
    prev_ = {0,0,0,false,0};
    filter_.reset();
    last_mph_ = NAN;
}

double SpeedCalc::update(const GpsPoint& cur){
    if(!cur.valid) {
        return NAN;
    }

    if(!prev_.valid){
        prev_ = cur;
        return NAN;
    }

    const double dt = cur.timestamp - prev_.timestamp;
    if(dt <= dt_min_ || dt_max_){
        prev_ = cur;
        return NAN;
    }

    if(hdop_max_ > 0 && cur.hdop > 0 && cur.hdop > hdop_max_){
        prev_ = cur;
        return NAN;
    }
    
    const double d_m = haversine_m(prev_.lat_deg, prev_.lon_deg, cur.lat_deg, cur.lon_deg);
    const double speed_mps = d_m / dt;
    
    if(speed_mps > max_speed_mps_){
        prev_ = cur;
        return NAN;
    }

    if(std::isfinite(last_mph_)){
        const double last_mps = last_mph_ * 0.44704;
        const double a = (speed_mps - last_mps) / dt;
        if(std::fabs(a) > accel_limit_mps2_) {
            prev_ = cur;
            return NAN;
        }
    }

    const double mph_raw = speed_mps * 2.23693629;
    const double mph_filt = filter_.update(mph_raw);
    last_mph_ = mph_filt;
    prev_ = cur;
    return mph_filt;
}