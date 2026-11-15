#include "../Source/GainRider.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
constexpr double pi = 3.14159265358979323846;
std::vector<float> sine(int n, float amp) {
    std::vector<float> x(n);
    for (int i=0;i<n;++i) x[i]=(float)(amp*std::sin(2*pi*440*i/48000.0));
    return x;
}
int main() {
    using namespace smoothride;
    GainRider rider;
    rider.prepare(48000);
    Settings s;
    rider.setSettings(s);
    auto left=sine(48000, 0.5f), right=left;
    rider.process(left.data(),right.data(),48000,true);
    for(int i=0;i<48000;++i) {
        assert(std::isfinite(left[i]));
        assert(std::abs(left[i])<=0.89126f);
        assert(left[i]==right[i]);
    }
    std::cout<<"PASS stereo linked, sample ceiling, finite\n";
    std::fill(left.begin(),left.end(),0); std::fill(right.begin(),right.end(),0);
    rider.process(left.data(),right.data(),48000,true);
    for(auto x:left) assert(x==0.0f);
    std::cout<<"PASS silence unchanged\n";
    rider.reset(); s.mix=0; s.outputDb=0; s.ceilingDb=-0.1f; rider.setSettings(s);
    left=sine(48000,0.1f); right=left;
    auto original=left[100]; rider.process(left.data(),right.data(),48000,true);
    assert(std::abs(left[100]-original)<1e-5);
    std::cout<<"PASS dry unity\n";
    // Independently bound boost and cut, with enough settling time for RMS detector.
    rider.reset(); s.mix=1; s.outputDb=0; s.maxBoostDb=6; s.maxCutDb=3;
    s.upMs=100; s.downMs=100; s.ceilingDb=-0.1f;
    rider.setSettings(s);
    left=sine(48000*4,0.015f);
    rider.process(left.data(),nullptr,(int)left.size(),false);
    assert(rider.currentRideDb()>5.5f && rider.currentRideDb()<=6.001f);
    assert(rider.isRidingActive());
    assert(rider.currentRequestedRideDb() > 5.9f);
    assert(rider.currentDetectedRmsDb() < s.targetDb);
    std::cout<<"PASS max boost limit and detector telemetry\n";
    rider.reset(); left=sine(48000*4,0.6f);
    rider.process(left.data(),nullptr,(int)left.size(),false);
    assert(rider.currentRideDb() < -2.5f && rider.currentRideDb()>=-3.001f);
    assert(rider.currentRequestedRideDb() < -2.9f);
    std::cout<<"PASS max cut limit and requested-gain telemetry\n";
    rider.reset();
    std::vector<float> silence(48000, 0.0f);
    rider.process(silence.data(), nullptr, (int)silence.size(), false);
    assert(!rider.isRidingActive());
    assert(rider.currentRequestedRideDb() == 0.0f);
    std::cout<<"PASS below-floor status\n";
}
