#ifndef GPS_SPEED_H
#define GPS_SPEED_H

#include <cmath>

struct GpsPoint {
    double lat_deg;
    double lon_deg;
    double timestamp;
    bool valid;
    float hdop;
};
double haversine_m(double lat1_deg, double lon1_deg,
                    double lat2_deg, double lon2_deg);
class SpeedEMA {
    public:
        explicit SpeedEMA(double alpha = 0.25);
        void reset();
        double update(double raw);
    private:
        double alpha_;
        bool inited_;
        double ema_;
};
class SpeedCalc{
    public:
        SpeedCalc(double alpha = 0.25);

        double update(const GpsPoint& cur);

        void reset();

        double last_mph() const {return last_mph_; }

        // Tuning knobs 
        void setAccelLimit(double mps2) { accel_limit_mps2_ = mps2; } // default ~12 m/s^2 (~1.2g)
        void setMaxSpeed(double mps)    { max_speed_mps_    = mps;  } // default 85 m/s (~190 mph)
        void setDtRange(double dtMin, double dtMax) { dt_min_ = dtMin; dt_max_ = dtMax; }
        void setHdopMax(float hdopMax)  { hdop_max_ = hdopMax; }      // default 3.0 (ignored if <=0)
    private:
    GpsPoint prev_;
    SpeedEMA filter_;
    double last_mph_;
    double accel_limit_mps2_;
    double max_speed_mps_;
    double dt_min_, dt_max_;
    float hdop_max_;
};
#endif