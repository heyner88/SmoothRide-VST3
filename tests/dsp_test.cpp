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
    // Optional soft-floor is off by default and limits boosts near the gate when enabled.
    GainRider baseline, protectedRider;
    baseline.prepare(48000); protectedRider.prepare(48000);
    Settings a; a.floorDb=-60.0f; a.targetDb=-12.0f; a.maxBoostDb=6.0f;
    a.upMs=100.0f; a.downMs=100.0f; a.mix=1.0f; a.outputDb=0.0f;
    Settings b=a; b.lowLevelProtect=1.0f;
    baseline.setSettings(a); protectedRider.setSettings(b);
    auto nearFloor=sine(48000*4,0.002f); // approx -60 dB RMS
    auto nearFloor2=nearFloor;
    baseline.process(nearFloor.data(),nullptr,(int)nearFloor.size(),false);
    protectedRider.process(nearFloor2.data(),nullptr,(int)nearFloor2.size(),false);
    assert(protectedRider.currentRideDb() < baseline.currentRideDb());
    std::cout<<"PASS soft floor reduces low-level boost\n";
    GainRider gated;
    gated.prepare(48000);
    Settings h=a; h.gateHysteresisDb=4.0f; h.floorDb=-40.0f;
    gated.setSettings(h);
    auto above=sine(48000*2,0.025f); // above -38 dB RMS
    gated.process(above.data(),nullptr,(int)above.size(),false);
    assert(gated.isRidingActive());
    auto middle=sine(48000*2,0.008f); // -43 dB RMS; below lower hysteresis edge (-42)
    gated.process(middle.data(),nullptr,(int)middle.size(),false);
    assert(!gated.isRidingActive());
    std::cout<<"PASS hysteresis exit threshold\n";
    // Adaptive floor mode must activate on quiet content without affecting
    // the historical default when disabled.
    Settings adaptiveSettings;
    adaptiveSettings.floorDb=-60.0f;
    adaptiveSettings.adaptiveFloor=1.0f;
    adaptiveSettings.outputDb=0.0f;
    adaptiveSettings.mix=1.0f;
    adaptiveSettings.upMs=100.0f;
    GainRider adaptiveRider;
    adaptiveRider.prepare(48000);
    adaptiveRider.setSettings(adaptiveSettings);
    auto quietProgram=sine(48000*2, 0.0003f); // about -73.5 dBFS RMS
    adaptiveRider.process(quietProgram.data(),nullptr,(int)quietProgram.size(),false);
    assert(adaptiveRider.isRidingActive());
    assert(adaptiveRider.currentEffectiveFloorDb() < -60.0f);
    assert(adaptiveRider.currentRideDb() > 1.0f);
    GainRider oldMode;
    oldMode.prepare(48000);
    adaptiveSettings.adaptiveFloor=0.0f;
    oldMode.setSettings(adaptiveSettings);
    auto quietManual=sine(48000*2, 0.0003f);
    oldMode.process(quietManual.data(),nullptr,(int)quietManual.size(),false);
    assert(!oldMode.isRidingActive());
    assert(oldMode.currentEffectiveFloorDb() == -60.0f);
    std::cout<<"PASS adaptive quiet program and manual compatibility\n";
    adaptiveRider.reset();
    adaptiveSettings.adaptiveFloor=1.0f;
    adaptiveRider.setSettings(adaptiveSettings);
    auto nearSilence=sine(48000*2, 0.00001f); // about -103 dBFS RMS
    adaptiveRider.process(nearSilence.data(),nullptr,(int)nearSilence.size(),false);
    assert(!adaptiveRider.isRidingActive());
    assert(adaptiveRider.currentRequestedRideDb() == 0.f);
    std::cout<<"PASS adaptive silence cutoff\n";
    // New guard must be transparent while OFF and when Adaptive Floor is OFF.
    Settings offGuard = adaptiveSettings;
    offGuard.adaptiveFloor = 1.f; offGuard.noiseGuard = 0.f;
    Settings disabledAdaptive = offGuard; disabledAdaptive.adaptiveFloor = 0.f; disabledAdaptive.noiseGuard = 1.f;
    GainRider reference, ignoredGuard;
    reference.prepare(48000); ignoredGuard.prepare(48000);
    Settings cleanManual = disabledAdaptive; cleanManual.noiseGuard = 0.f;
    reference.setSettings(cleanManual);
    ignoredGuard.setSettings(disabledAdaptive);
    auto identical=sine(48000, .0003f), identical2=identical;
    reference.process(identical.data(),nullptr,(int)identical.size(),false);
    ignoredGuard.process(identical2.data(),nullptr,(int)identical2.size(),false);
    assert(identical == identical2);
    assert(!ignoredGuard.isNoiseBlocked());
    std::cout<<"PASS noise guard bypass in manual mode\n";
    GainRider guarded;
    guarded.prepare(48000);
    offGuard.noiseGuard=1.f;
    guarded.setSettings(offGuard);
    auto steady=sine(48000*40, .0003f);
    guarded.process(steady.data(),nullptr,(int)steady.size(),false);
    assert(guarded.isNoiseBlocked());
    assert(!guarded.isRidingActive());
    std::cout<<"PASS experimental guard blocks long steady quiet program\n";
}
